#include "tesseract_planning/ompl/chain_ompl_interface.h"
#include <ompl/base/spaces/RealVectorStateSpace.h>

namespace tesseract
{
namespace tesseract_planning
{
ChainOmplInterface::ChainOmplInterface(tesseract::BasicEnvConstPtr environment, const std::string& manipulator_name)
  : env_(std::move(environment))
{
  if (!env_->hasManipulator(manipulator_name))
    throw std::invalid_argument("No such manipulator " + manipulator_name + " in Tesseract environment");

  const auto& manip = env_->getManipulator(manipulator_name);
  joint_names_ = manip->getJointNames();
  link_names_ = manip->getLinkNames();

  const auto dof = manip->numJoints();
  const auto limits = manip->getLimits();

  // Construct the OMPL state space for this manipulator
  ompl::base::RealVectorStateSpace* space = new ompl::base::RealVectorStateSpace();
  for (unsigned i = 0; i < dof; ++i)
  {
    space->addDimension(joint_names_[i], limits(i, 0), limits(i, 1));
  }

  ompl::base::StateSpacePtr state_space_ptr(space);
  ss_.reset(new ompl::geometric::SimpleSetup(state_space_ptr));

  // Setup state checking functionality
  ss_->setStateValidityChecker(std::bind(&ChainOmplInterface::isStateValid, this, std::placeholders::_1));
  contact_fn_ = std::bind(&ChainOmplInterface::isContactAllowed, this, std::placeholders::_1, std::placeholders::_2);

  tesseract::ContactRequest req;
  req.link_names = link_names_;
  req.isContactAllowed = contact_fn_;
  req.type = tesseract::ContactRequestTypes::FIRST;

  contact_manager_ = env_->getDiscreteContactManager();
  contact_manager_->setContactRequest(req);

  // We need to set the planner and call setup before it can run
}

boost::optional<ompl::geometric::PathGeometric> ChainOmplInterface::plan(ompl::base::PlannerPtr planner,
                                                                         const std::vector<double>& from,
                                                                         const std::vector<double>& to,
                                                                         const OmplPlanParameters& params)
{
  ss_->setPlanner(planner);
  planner->clear();

  const auto dof = ss_->getStateSpace()->getDimension();
  ompl::base::ScopedState<> start_state(ss_->getStateSpace());
  for (unsigned i = 0; i < dof; ++i)
    start_state[i] = from[i];

  ompl::base::ScopedState<> goal_state(ss_->getStateSpace());
  for (unsigned i = 0; i < dof; ++i)
    goal_state[i] = to[i];

  ss_->setStartAndGoalStates(start_state, goal_state);

  ompl::base::PlannerStatus status = ss_->solve(params.planning_time);

  if (status)
  {
    if (params.simplify)
    {
      ss_->simplifySolution();
    }

    ompl::geometric::PathGeometric& path = ss_->getSolutionPath();
    return boost::optional<ompl::geometric::PathGeometric>{ path };
  }
  else
  {
    return {};
  }
}

ompl::base::SpaceInformationPtr ChainOmplInterface::spaceInformation() { return ss_->getSpaceInformation(); }
bool ChainOmplInterface::isStateValid(const ompl::base::State* state) const
{
  const ompl::base::RealVectorStateSpace::StateType* s = state->as<ompl::base::RealVectorStateSpace::StateType>();
  const auto dof = joint_names_.size();

  Eigen::Map<Eigen::VectorXd> joint_angles(s->values, dof);
  tesseract::EnvStatePtr env_state = env_->getState(joint_names_, joint_angles);

  for (const auto& link_name : link_names_)
    contact_manager_->setCollisionObjectsTransform(link_name, env_state->transforms[link_name]);

  tesseract::ContactResultMap contact_map;
  contact_manager_->contactTest(contact_map);

  return contact_map.empty();
}

bool ChainOmplInterface::isContactAllowed(const std::string& a, const std::string& b) const
{
  return env_->getAllowedCollisionMatrix()->isCollisionAllowed(a, b);
}
}
}
