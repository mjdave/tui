//
//  main.cpp
//  mjs
//
//  Created by David Frampton on 18/04/2025.
//

#include <iostream>
#include "Interpreter.h"
#include "MJLog.h"

int main(int argc, const char * argv[]) {
    
    std::vector<std::string> args;
    for(int i = 1; i < argc; i++)
    {
        args.push_back(argv[i]);
    }
    
    if(args.empty())
    {
        MJLog("Please supply a script to run, eg. ./mjs examples/example.mjh");
        return 0;
    }
    
    Interpreter* interpreter = new Interpreter(args);
    delete interpreter;
    
    return 0;
}


/*
 //
 //  main.cpp
 //  WorldServer
 //
 //  Created by David Frampton on 2/12/14.
 //  Copyright (c) 2014 Majic Jungle. All rights reserved.
 //

 #include <iostream>
 #include "ServerAppController.h"
 #include <getopt.h>
 #include "StringUtils.h"
 #include "NetConstants.h"
 #include "MJVersion.h"

 static const struct option longopts[] = {
     { "list",               no_argument, NULL, 'l' },
     { "help",               no_argument, NULL, 'h' },
     { "yes",                no_argument, NULL, 'y' },
     { "yes-upload-world",   no_argument, NULL, 'Y' },
     
     { "advertise",          no_argument, NULL, 'a' },
     
     { "new",                required_argument, NULL, 'n' },
     { "seed",               required_argument, NULL, 's' },
     { "load",               required_argument, NULL, 'o' },
     { "port",               required_argument, NULL, 'p' },
     { "http-port",          required_argument, NULL, 'P' },
     { "server-id",          required_argument, NULL, 'u' },
     { "savedir",            required_argument, NULL, 'd' },
 };



 void printErrorForMissingArgForOption(int option)
 {
     switch (option) {
         case 'n':
         {
             printf("--new must be followed by the name of the world you would like to create.\n");
         }
             break;
         case 'o':
         {
             printf("--load must be followed by the name of the world you would like to load.\n");
         }
             break;
         case 'p':
         {
             printf("--port must be followed by the port you would like the server to run on. Default is 16161.\n");
         }
             break;
         case 'P':
         {
             printf("--http-port must be followed by the port you would like the http server to run on. Default is 16168.\n");
         }
             break;
         case 's':
         {
             printf("--seed must be followed by a seed string to use for world generation.\n");
         }
             break;
         case 'u':
         {
             printf("--server-id must be followed by a string to uniquely identify the server.\n");
         }
             break;
         case 'd':
         {
             printf("--savedir must be followed by a directory name string to define where world data will be saved.\n");
         }
             break;
             
         default:
             break;
     }
 }

 int main(int argc, const char * argv[]) {
     
     int option_index = 0;
     int opt = 0;
     
     std::string newName;
     std::string loadName;
     std::string newSeed;
     std::string port;
     std::string httpPort;
     std::string playerID;
     std::string savedir;
     
     bool advertise = false;
     bool listWorlds = false;
     bool sendBugReports = false;
     bool sendBugReportWorlds = false;
     
     while ((opt = getopt_long_only(argc, (char **)argv, "n:o:p:P:s:u:d:alhyY",
                                    longopts, &option_index)) != -1)
     {
         
         switch (opt) {
             case 'n':
             {
                 if(optarg)
                 {
                     newName = optarg;
                 }
             }
                 break;
             case 'o':
             {
                 if(optarg)
                 {
                     loadName = optarg;
                 }
             }
                 break;
             case 'p':
             {
                 if(optarg)
                 {
                     port = optarg;
                 }
             }
                 break;
             case 'P':
             {
                 if(optarg)
                 {
                     httpPort = optarg;
                 }
             }
                 break;
             case 's':
             {
                 if(optarg)
                 {
                     newSeed = optarg;
                 }
             }
                 break;
             case 'a':
             {
                 advertise = true;
             }
                 break;
             case 'u':
             {
                 if(optarg)
                 {
                     playerID = optarg;
                 }
             }
                 break;
             case 'd':
             {
                 if(optarg)
                 {
                     savedir = optarg;
                 }
             }
                 break;
             case 'l':
             {
                 listWorlds = true;
             }
                 break;
             case 'h':
             {
                 std::string usageString = string_format("\n\n\
 Usage: ./server [options]\n\
 \n\
 Version:%s\n\
 \n\
 When run without options, the most recent world will be loaded.\n\
 Available options:\n\
 \t--list (-l) list all available worlds.\n\
 \t--new WORLD_NAME (-n) create a new world named WORLD_NAME.\n\
 \t--seed WORLD_SEED (-s) use WORLD_SEED when creating new world.\n\
 \t--load WORLD_ID [options] (-o) load the world with the id WORLD_ID.\n\
 \t--port PORT (-p) the port to run on when specified with --new or --load. Default is 16161. Sapiens requires two UDP ports, so the next port above this will also be used (eg 16162).\n\
 \t--http-port HTTP_PORT (-P) the port for the http server to run on when specified with --new or --load. Default is 16168.\n\
 \t--advertise (-a) Make this server visible in the server list on the game client. This makes your server publicly visible so anyone can easily find your server and connect to it without knowing the IP or port. To change the name displayed in the public server list, modify advertiseName in config.lua inside the world save directory.\n\
 \t--server-id SERVER_ID (-u) can optionally be supplied to control the player directory name where the worlds are saved. Should ideally be unique to the server, but can be anything that can be used as a directory name. If not supplied, a random string is generated and used.\n\
 \t--savedir SAVE_PATH (-u) can optionally be used to set the base parent directory for all save data. This directory must exist, a directory named 'sapiens' will be created within.\n\
 \t--yes (-y) opt in to automatically send bug reports to the developer. Please do pass this argument or -Y if you don't mind helping the developer to fix bugs in Sapiens!\n\
 \t--yes-upload-world (-Y) opt in to automatically send bug reports to the developer, and also include a world save in the bug report package. This option should be used only when a world save is requested by the developer to help diagnose a particular issue.\n\
 \t--help (-h) display this help text\n", VERSION_STRING);
                 
                 printf("%s\n", usageString.c_str());
                 return 0;
             }
                 break;
             case 'y':
             {
                 sendBugReports = true;
             }
                 break;
             case 'Y':
             {
                 sendBugReportWorlds = true;
             }
                 break;
             case '?':
             {
                 printErrorForMissingArgForOption(optopt);
                 return 0;
             }
                 break;
                 
             default:
                 break;
         }
     }
     
     new ServerAppController(newName,
                             newSeed,
                             loadName,
                             port,
                             httpPort,
                             savedir,
                             advertise,
                             playerID,
                             listWorlds,
                             sendBugReports || sendBugReportWorlds,
                             sendBugReportWorlds);
     
     
     return 0;
 }

 */
