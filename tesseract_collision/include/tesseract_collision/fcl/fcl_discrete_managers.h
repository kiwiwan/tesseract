/**
 * @file fcl_discrete_managers.h
 * @brief Tesseract ROS FCL contact checker implementation.
 *
 * @author Levi Armstrong
 * @date Dec 18, 2017
 * @version TODO
 * @bug No known bugs
 *
 * @copyright Copyright (c) 2017, Southwest Research Institute
 *
 * @par License
 * Software License Agreement (BSD)
 * @par
 * All rights reserved.
 * @par
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * @par
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * @par
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TESSERACT_COLLISION_FCL_DISCRETE_MANAGERS_H
#define TESSERACT_COLLISION_FCL_DISCRETE_MANAGERS_H

#include <tesseract_core/discrete_contact_manager_base.h>
#include <tesseract_collision/fcl/fcl_utils.h>

namespace tesseract
{
/** @brief A FCL implementation of the discrete contact manager */
class FCLDiscreteBVHManager : public DiscreteContactManagerBase
{
public:
  FCLDiscreteBVHManager();

  DiscreteContactManagerBasePtr clone() const override;

  bool addCollisionObject(const std::string& name,
                          const int& mask_id,
                          const std::vector<shapes::ShapeConstPtr>& shapes,
                          const VectorIsometry3d& shape_poses,
                          const CollisionObjectTypeVector& collision_object_types,
                          bool enabled = true) override;

  bool hasCollisionObject(const std::string& name) const override;

  bool removeCollisionObject(const std::string& name) override;

  bool enableCollisionObject(const std::string& name) override;

  bool disableCollisionObject(const std::string& name) override;

  void setCollisionObjectsTransform(const std::string& name, const Eigen::Isometry3d& pose) override;

  void setCollisionObjectsTransform(const std::vector<std::string>& names,
                                    const VectorIsometry3d& poses) override;

  void setCollisionObjectsTransform(const TransformMap& transforms) override;

  void contactTest(ContactResultMap& collisions) override;

  void setContactRequest(const ContactRequest& req) override;

  const ContactRequest& getContactRequest() const override;

  /**
   * @brief Add a fcl collision object to the manager
   * @param cow The tesseract fcl collision object
   */
  void addCollisionObject(const FCLCOWPtr& cow);

  /**
   * @brief Return collision objects
   * @return A map of collision objects <name, collision object>
   */
  const Link2FCLCOW& getCollisionObjects() const;

private:
  std::unique_ptr<fcl::BroadPhaseCollisionManagerd> manager_; /**< @brief FCL Broad Phase Collision Manager */
  Link2FCLCOW link2cow_;                                      /**< @brief A map of all (static and active) collision objects being managed */
  ContactRequest request_;                                    /**< @brief Active request to be used for methods that don't require a request */

};
typedef std::shared_ptr<FCLDiscreteBVHManager> FCLDiscreteBVHManagerPtr;

}
#endif // TESSERACT_COLLISION_FCL_DISCRETE_MANAGERS_H
