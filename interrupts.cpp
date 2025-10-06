/**
 *
 * @file interrupts.cpp
 * @author Cole Galway 
 * @author Taylor Brumwell
 *
 */

#include "interrupts.hpp"

int main(int argc, char** argv) {

    //vectors is a C++ std::vector of strings that contain the address of the ISR
    //delays  is a C++ std::vector of ints that contain the delays of each device
    //the index of these elemens is the device number, starting from 0
    auto [vectors, delays] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    std::string trace;      //!< string to store single line of trace file
    std::string execution;  //!< string to accumulate the execution output

    /******************ADD YOUR VARIABLES HERE*************************/
    int current_time = 0; //tracks the time as the program continues

    int context_time = 10; //value to save and restore the og clock-time
    int isr_activity_time = 40; //each ISR block timing

    int total_overhead_time = 0; //overall duration of overhead work thoughout the whole program
    int total_user_time = 0; //overall duration of the user application throughout the whole program (everything except overhead work)

    /******************************************************************/

    //parse each line of the input trace file
    while(std::getline(input_file, trace)) {
        auto [activity, duration_intr] = parse_trace(trace);

        /******************ADD YOUR SIMULATION CODE HERE*************************/
        //handle when the CPU is being used
        if (activity == "CPU") {
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + "CPU burst\n";
            total_user_time += duration_intr;
            current_time += duration_intr;
        }

        //SYSCALL
        else if (activity == "SYSCALL") {
            int device_num = duration_intr;

            if (device_num >= vectors.size()) {
                std::cerr << "Error: device_num " << device_num << " is invalid for vectors (max index = " << vectors.size()-1 << ")\n";
                continue;
            }

            auto [intr_text, new_time] = intr_boilerplate(current_time, device_num, context_time, vectors);
            execution += intr_text;
            current_time = new_time;

            if (device_num >= delays.size()) {
                std::cerr << "Error: device_num " << device_num  << " is invalid (max index = " << delays.size()-1 << ")\n";
                continue;
            }

            int remaining = delays.at(device_num);
            while(remaining > 0) {
                int chunk = std::min(remaining, isr_activity_time);
                execution += std::to_string(current_time) + ", " + std::to_string(chunk) + " ,call device driver\n";
                current_time += chunk;
                remaining -= chunk;
            }

            execution += std::to_string(current_time) + ", 1, IRET\n";
            total_overhead_time += 1;
            current_time += 1;

            execution += std::to_string(current_time) + ", " + std::to_string(context_time) + ", context restored\n";
            total_overhead_time += context_time;
            current_time += context_time;

            execution += std::to_string(current_time) + ", 1, switch to user mode\n";
            total_overhead_time += 1;
            current_time = 1;
        }

        //END_IO
        else if (activity == "END_IO") {
            int device_num = duration_intr;

            if (device_num >= vectors.size()) {
                std::cerr << "Error: device_num " << device_num << " is invalid for vectors (max index = " << vectors.size()-1 << ")\n";
                continue;
            }

            auto [intr_text, new_time] = intr_boilerplate(current_time, device_num, context_time, vectors);
            execution += intr_text;
            current_time = new_time;

            if (device_num >= delays.size()) {
                std::cerr << "Error: device_num " << device_num  << " is invalid (max index = " << delays.size()-1 << ")\n";
                continue;
            }

            int device_delay = delays.at(device_num);
            int isr_step_time = 40;
            int remaining = device_delay;

            while(remaining > 0) {
                int chunk = std::min(remaining, isr_activity_time);
                execution += std::to_string(current_time) + ", " + std::to_string(chunk) + " ,ISR body for end_io" + std::to_string(device_num) + "\n";
                current_time += chunk;
                remaining -= chunk;
            }

            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

            execution += std::to_string(current_time) + ", " + std::to_string(context_time) + ", context restored\n";
            current_time += context_time;

            execution += std::to_string(current_time) + ", 1, switch to user mode\n";
            current_time = 1;

            total_overhead_time += context_time;
            total_overhead_time += isr_activity_time;
            total_overhead_time += context_time;
        }

        if (input_file.peek() == EOF) {
            execution += "\n=== Simulation Summary ===\n";
            execution += "User time: " + std::to_string(total_user_time) + " ms\n";
            execution += "Overhead time: " + std::to_string(total_overhead_time) + " ms\n";
            execution += "Total time: " + std::to_string(total_user_time + total_overhead_time) + " ms\n";
        }


        /************************************************************************/

    }

    input_file.close();

    write_output(execution);

    return 0;
}
