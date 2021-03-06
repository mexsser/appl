/**
  @brief Controller console for APPL&ROS wrapper
  @author Shaoujun Cai (based on Haoyu Bai's work)
  @date 2013-05-08
**/


#include "ros/ros.h"
#include "Controller.h"
#include "GlobalResource.h"
#include "MOMDP.h"
#include "ParserSelector.h"
#include "AlphaVectorPolicy.h"
#include "appl/appl_request.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
using namespace std;

Controller *p_control;
map<string, int> obsSymbolMapping, obsStateMapping;

/*service callback function to process the command from client*/
bool servicefn(appl::appl_request::Request&req,appl::appl_request::Response&res)
{
  	//command 0,1,2   ping init request
  	int command=req.cmd;
  	string xstate=req.xstate;
  	string obs=req.obs;
  	int action;
	if (command == 0) {
	    res.action=-1;	//for testing
	} else if (command==1) {
	    // format: ".init xstate"
	    int idx_xstate = obsStateMapping[xstate];
	    p_control->reset(idx_xstate);
	    // dummy obs for first action
	    action = p_control->nextAction(0, 0);
	    cout << "action:" << action << endl;
	    res.action=action;
	} else {
	    int idx_xstate = obsStateMapping[xstate];
	    int idx_obs = obsSymbolMapping[obs];
	    action = p_control->nextAction(idx_obs, idx_xstate);
	    res.action=action;
	    cout << "action:" << action << endl;
	}
	cout << "Current belief:";
	p_control->currBelief()->bvec->write(cout);
	cout << endl;
	ROS_INFO("action service ended");
	return true;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "controlpipe");
    ros::NodeHandle nh;
    ros::ServiceServer service = nh.advertiseService("appl_request", servicefn);
    std::string problem_name, policy_name, default_param;
    nh.getParam("/controlpipe/appl_problem_name", problem_name);
    nh.getParam("/controlpipe/appl_policy_name", policy_name);
    ROS_INFO("%s!!!!!!!!!!!!!!!!!!",problem_name.c_str());
    
    
    SolverParams* p = &GlobalResource::getInstance()->solverParams;
    setvbuf(stdout,0, _IONBF,0);
    setvbuf(stderr,0, _IONBF,0);
    p->policyFile=policy_name;
    p->problemName=problem_name;
    

    cout<<"\nLoading the model ...\n   ";
    SharedPointer<MOMDP> problem = ParserSelector::loadProblem(p->problemName, *p);

    SharedPointer<AlphaVectorPolicy> policy = new AlphaVectorPolicy(problem);

    cout<<"\nLoading the policy ... input file : "<<p->policyFile<<"\n";
    bool policyRead = policy->readFromFile(p->policyFile);

    if (p->useLookahead) {
        cout<<"   action selection : one-step look ahead\n";
    }

    // TODO we assume the initial observed state is 0 for now
    p_control=new Controller(problem,policy,p,0);

    /// Mapping observations for quick reference

    for(int i = 0 ; i < problem->XStates->size() ; i ++)
    {   
        //mapFile << "State : " << i <<  endl;
        //cout << "State : " << i <<  endl;
        map<string, string> obsState = problem->getFactoredObservedStatesSymbols(i);
        string state_str;
        for(map<string, string>::iterator iter = obsState.begin() ; iter != obsState.end() ; iter ++)
        {   
            //cout << iter->first << " : " << iter->second << endl ;
            state_str.append(iter->second);
        }
        obsStateMapping[state_str]=i;
    }

    for(int i = 0 ; i < problem->observations->size() ; i ++)
    {
        map<string, string> obsSym = problem->getObservationsSymbols(i);
        string obs_str;
        for(map<string, string>::iterator iter = obsSym.begin() ; iter != obsSym.end() ; iter ++)
        {
            obs_str.append(iter->second);
        }
        obsSymbolMapping[obs_str]=i;
    }
    

    cout<<"\nReady.\n";
     
    while(true)
    {
    	ros::spin();
    }

    return 0;
}
