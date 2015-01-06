// class header
#include <gua/renderer/SkeletalAnimationDirector.hpp>


// guacamole headers
#include <gua/utils/Logger.hpp>

//external headers
#include <iostream>

namespace gua 
{
SkeletalAnimationDirector::SkeletalAnimationDirector(aiScene const* scene):
    num_bones_{0},
    has_anims_{scene->HasAnimations()},
    firstRun_{true},
    animations_{},
    currAnimation_{},
    animNum{0},
    anim_start_node_{},
    state_{Playback::crossfade},
    blending_state_{Blending::linear},
    next_transition_{0},
    timer_{} {
  timer_.start();
  add_hierarchy(scene);
}

void SkeletalAnimationDirector::add_hierarchy(aiScene const* scene) {
  root_ = anim_start_node_ = SkeletalAnimationUtils::load_hierarchy(scene);
  SkeletalAnimationUtils::collect_bone_indices(bone_mapping_, root_);
  num_bones_ = bone_mapping_.size();

  anim_start_node_ = root_->children[1]->children[0]->children[0];
}

void SkeletalAnimationDirector::add_animations(aiScene const* scene) {
  std::vector<std::shared_ptr<SkeletalAnimation>> newAnims{SkeletalAnimationUtils::load_animations(scene)};

  animations_.reserve(animations_.size() + newAnims.size());
  std::move(newAnims.begin(), newAnims.end(), std::inserter(animations_, animations_.end()));
  newAnims.clear();

  has_anims_ = animations_.size() > 0;
  currAnimation_ = animations_[animations_.size()-1];
}

void SkeletalAnimationDirector::blend_pose(float timeInSeconds, float blendFactor, std::shared_ptr<SkeletalAnimation> const& pAnim1, std::shared_ptr<SkeletalAnimation> const& pAnim2, std::vector<scm::math::mat4f>& transforms) {

  float timeNormalized1 = timeInSeconds / pAnim1->duration;
  timeNormalized1 = scm::math::fract(timeNormalized1);

  float timeNormalized2 = timeInSeconds / pAnim2->duration;
  timeNormalized2 = scm::math::fract(timeNormalized2);

  scm::math::mat4f identity = scm::math::mat4f::identity();

  Pose pose1{SkeletalAnimationUtils::calculate_pose(timeNormalized1, pAnim1)};
  Pose pose2{SkeletalAnimationUtils::calculate_pose(timeNormalized2, pAnim2)};
  
  pose1.blend(pose2, blendFactor);

  SkeletalAnimationUtils::accumulate_matrices(transforms, anim_start_node_, pose1, identity);
}

void SkeletalAnimationDirector::partial_blend(float timeInSeconds, std::shared_ptr<SkeletalAnimation> const& pAnim1, std::shared_ptr<SkeletalAnimation> const& pAnim2, std::string const& nodeName, std::vector<scm::math::mat4f>& transforms) {
  
  std::shared_ptr<Node> start{};
  start = SkeletalAnimationUtils::find_node(nodeName, root_->children[1]->children[0]->children[0]);
  
  if(!start) {
    Logger::LOG_WARNING << "node '"<< nodeName << "' not found" << std::endl; 
    return;
  }

  float timeNormalized1 = timeInSeconds / pAnim1->duration;
  timeNormalized1 = scm::math::fract(timeNormalized1);

  float timeNormalized2 = timeInSeconds / pAnim2->duration;
  timeNormalized2 = scm::math::fract(timeNormalized2);

  scm::math::mat4f identity = scm::math::mat4f::identity();

  Pose full_body{SkeletalAnimationUtils::calculate_pose(timeNormalized1, pAnim1)};
  Pose upper_body{SkeletalAnimationUtils::calculate_pose(timeNormalized2, pAnim2)};

  full_body.partial_replace(upper_body, start);

  SkeletalAnimationUtils::accumulate_matrices(transforms, anim_start_node_, full_body, identity);
}

std::vector<scm::math::mat4f> SkeletalAnimationDirector::get_bone_transforms()
{
  //reserve vector for transforms
  std::vector<scm::math::mat4f> transforms{num_bones_, scm::math::mat4f::identity()};
  
  float currentTime = timer_.get_elapsed();

  if(!has_anims_) {
    SkeletalAnimationUtils::calculate_matrices(0, anim_start_node_, currAnimation_, transforms);
    return transforms;  
  }

  switch(state_) {
    //crossfade two anims
    case Playback::crossfade: {
      float blendDuration = 2;
      float playDuration = animations_[animNum]->duration;

      if(currentTime < next_transition_) {
        SkeletalAnimationUtils::calculate_matrices(currentTime, anim_start_node_, animations_[animNum], transforms);  
      }
      else if(currentTime <= next_transition_ + blendDuration) {
        float time = (currentTime - next_transition_) / blendDuration;
        
        float blendFactor = 0.0;
        switch(blending_state_){
          case Blending::swap : blendFactor = Blend::swap(time);break;
          case Blending::linear : blendFactor = Blend::linear(time);break;
          case Blending::smoothstep : blendFactor = Blend::smoothstep(time);break;
          case Blending::cosinus : blendFactor = Blend::cos(time);break;
          default: blendFactor = Blend::linear(time);
        }
        
        blend_pose(currentTime, blendFactor, animations_[animNum], animations_[(animNum + 1) % animations_.size()], transforms);
      }
      else {
        next_transition_ = currentTime + playDuration;
        animNum = (animNum + 1) % animations_.size();
      }
      break;
    }
    //blend two anims
    case Playback::partial: {
      partial_blend(currentTime, animations_[0], animations_[1], "Waist", transforms);
      break;
    }
    default: Logger::LOG_WARNING << "playback mode not found" << std::endl; break;
  }
  
  return transforms;
}

void SkeletalAnimationDirector::set_playback_mode(uint mode) {
  switch(mode) {
    case 0 : state_ = Playback::partial; break;
    case 1 : state_ = Playback::crossfade; break;
    default: state_ = Playback::partial;;
  }
}

uint SkeletalAnimationDirector::get_playback_mode() {
  return state_;
}

void SkeletalAnimationDirector::set_blending_mode(uint mode) {
  switch(mode) {
    case 0 : blending_state_ = Blending::swap; break;
    case 1 : blending_state_ = Blending::linear; break;
    case 2 : blending_state_ = Blending::smoothstep; break;
    case 3 : blending_state_ = Blending::cosinus; break;
    default: blending_state_ = Blending::linear;;
  }
}

uint SkeletalAnimationDirector::get_blending_mode() {
  return blending_state_;
}

int SkeletalAnimationDirector::getBoneID(std::string const& name) {
  return bone_mapping_.at(name);
}

bool SkeletalAnimationDirector::has_anims() const {
  return has_anims_;
}

} // namespace gua