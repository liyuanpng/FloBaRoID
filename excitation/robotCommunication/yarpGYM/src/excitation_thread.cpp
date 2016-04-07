#include <yarp/os/all.h>

#include "excitation_thread.h"
#include "excitation_constants.h"
#include "saveEigen.h"

#include <Eigen/Eigen>

#include <list>
#include <vector> 

using namespace std;
using namespace Eigen;
list < VectorXd > tmpLog; 
excitation_thread::excitation_thread(   std::string module_prefix,
                                        yarp::os::ResourceFinder rf,
                                        std::shared_ptr< paramHelp::ParamHelperServer > ph) :
                                            left_arm_chain_interface( "left_arm", module_prefix, get_robot_name() ),
                                            num_joints( left_arm_chain_interface.getNumberOfJoints() ),
                                            ref_speed_vector( num_joints ),
                                            command_interface( module_prefix ),
                                            generic_thread( module_prefix, rf, ph )
{
    // position mode on left arm chain interface
    left_arm_chain_interface.setPositionDirectMode();   //sets setpoint directly, uses maximum speed all the time
    //left_arm_chain_interface.setPositionMode();   //generate trajectories, allows setting velocities
    //left_arm_chain_interface.setVelocityMode();

    //init velocity control with zero velocity
    /*yarp::sig::Vector vels;
    if(vels.size()!=num_joints) vels.resize(num_joints);
    vels.zero();
    left_arm_chain_interface.move(vels);*/

    //open data output port
    outgoingPort.open("/" + module_prefix + "/state:o");
}

excitation_thread::~excitation_thread()
{
    //save(&tmpLog, "log.csv");
}

void excitation_thread::link_tutorial_params()
{
    // get a shared pointer to param helper
    std::shared_ptr<paramHelp::ParamHelperServer> ph = get_param_helper();
    // link parameters (single value linking)
    //ph->linkParam( PARAM_ID_N_DOFS, &n_dofs );
}

bool excitation_thread::custom_init()
{
    // link the tutorial additional params to param helper
    link_tutorial_params();

    return true;
}

void excitation_thread::run()
{
    static yarp::sig::Vector pos;
    if(pos.size()!=num_joints) pos.resize(num_joints);
    static yarp::sig::Vector vels;
    if(vels.size()!=num_joints) vels.resize(num_joints);

    static yarp::sig::Vector q_left_arm, qdot_left_arm, tau_left_arm;
    double t = yarp::os::Time::now();

    /*
    double l=1, w_f=1, a=0.5, b=0.8, q0=-35, nf=1;
    double angle = (a/(w_f*l)*sin(w_f*l*t) +  a/(w_f*l)*cos(w_f*l*t))/M_PI*180;

    if(pos.size()!=num_joints) pos.resize(num_joints);
    pos.zero();
    /*pos[0] = excitation_cmd.angle0;
    pos[1] = excitation_cmd.angle1;
    pos[2] = angle;
    pos[3] = angle;
    pos[4] = excitation_cmd.angle4;
    pos[5] = excitation_cmd.angle5;
    pos[6] = excitation_cmd.angle6;
    left_arm_chain_interface.move(pos);
    */

    static int seq_num;
    excitation_cmd.command = "";
    command_interface.getCommand(excitation_cmd, seq_num);

    if( excitation_cmd.command == "set_left_arm" ) {
        // position move to desired configuration
        pos[0] = excitation_cmd.angle0;
        pos[1] = excitation_cmd.angle1;
        pos[2] = excitation_cmd.angle2;
        pos[3] = excitation_cmd.angle3;
        pos[4] = excitation_cmd.angle4;
        pos[5] = excitation_cmd.angle5;
        pos[6] = excitation_cmd.angle6;

        vels[0] = excitation_cmd.velocity0;
        vels[1] = excitation_cmd.velocity1;
        vels[2] = excitation_cmd.velocity2;
        vels[3] = excitation_cmd.velocity3;
        vels[4] = excitation_cmd.velocity4;
        vels[5] = excitation_cmd.velocity5;
        vels[6] = excitation_cmd.velocity6;

        //left_arm_chain_interface.setReferenceSpeeds(vels);
        left_arm_chain_interface.move(pos);

        //left_arm_chain_interface.move(vels);

        // get state data and send
        left_arm_chain_interface.sensePosition(q_left_arm);     //get positions in deg
        left_arm_chain_interface.senseVelocity(qdot_left_arm);  //get velocities in deg/s
        left_arm_chain_interface.senseTorque(tau_left_arm);     //get torques in Nm

        yarp::os::Bottle out_bottle;
        yarp::os::Bottle &out0 = out_bottle.addList();
        yarp::os::Bottle &out1 = out_bottle.addList();
        yarp::os::Bottle &out2 = out_bottle.addList();

        for(int i=0; i<q_left_arm.size(); i++){
            out0.addDouble(q_left_arm[i]);
            out1.addDouble(qdot_left_arm[i]);
            out2.addDouble(tau_left_arm[i]);
        }
        out_bottle.addDouble(t);

        if(outgoingPort.isOpen()) {
            outgoingPort.write(out_bottle);
        }
    }

    /*else if( excitation_cmd.command == "get_left_arm_measurements" ) {
        left_arm_chain_interface.sensePosition(q_left_arm);     //get positions in deg
        left_arm_chain_interface.senseVelocity(qdot_left_arm);  //get velocities in deg/s
        left_arm_chain_interface.senseTorque(tau_left_arm);     //get torques in Nm

        yarp::os::Bottle out_bottle;
        yarp::os::Bottle &out0 = out_bottle.addList();
        yarp::os::Bottle &out1 = out_bottle.addList();
        yarp::os::Bottle &out2 = out_bottle.addList();

        for(int i=0; i<q_left_arm.size(); i++){
            out0.addDouble(q_left_arm[i]);
            out1.addDouble(qdot_left_arm[i]);
            out2.addDouble(tau_left_arm[i]);
        }
        out_bottle.addDouble(t);

        if(outgoingPort.isOpen()) {
            outgoingPort.write(out_bottle);
        }

        yarp::sig::Vector out;
        out.resize(num_joints*3+1);
        out.setSubvector(0, q_left_arm);
        out.setSubvector(num_joints, qdot_left_arm);
        out.setSubvector(num_joints*2, tau_left_arm);
        out[num_joints*3] = t;

        static VectorXd tmp(out.size());
        for (int i=0; i<tmp.rows(); i++)
          tmp(i) = out(i);
        tmpLog.push_back(tmp);        
    }*/

    else if( excitation_cmd.command != "" ) {
        std::cout << excitation_cmd.command <<  " -> command not valid" << std::endl;
    }
}