#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <regex>

const static char *logfile_path = "LOGFILE_PATH";
const static std::string username = "USERNAME";

using std::string;

template <typename Func>
void log_main(Func callback)
{
    string line;

    std::ifstream logfile(logfile_path);

    logfile.seekg(0, logfile.end);
    int old_len = logfile.tellg();
    int new_len;
    logfile.close();

    while (true)
    {
        logfile.open(logfile_path);
        logfile.seekg(0, logfile.end);
        new_len = logfile.tellg();

        if (old_len < new_len)
        {
            logfile.seekg(old_len);
            getline(logfile, line);
            // printf("new line: %s\n", line.c_str());
            old_len = logfile.tellg();

            std::regex command_regex("^(?:\\*DEAD\\* )?" + username + " :  (.*)$");

            bool is_from_me = std::regex_search(line.c_str(), command_regex);
            if (is_from_me)
            {
                // puts("received message from me");
                string new_command = std::regex_replace(line.c_str(), command_regex, "$1");
                if (new_command.at(0) == '!')
                {
                    // puts("processing command");
                    callback(new_command);
                }
            }
        }
        sleep(1);
        logfile.close();
    }
    logfile.close();
}