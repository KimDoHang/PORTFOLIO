import argparse
from ast import parse
import jinja2
import MarkinosParser
import os


def main():
    
    
    parser = argparse.ArgumentParser(description = "MarkinosC Parser")
    defFolder = os.getcwd()
    parser.add_argument("--path", type=str, help ="MIDL File Path", default = defFolder+"\\Test.txt")
    parser.add_argument("--output", type=str, help="Output File Path", default = defFolder + "\\")
    parser.add_argument("--client", type=str, help="Client Class Name", default = "Markinos_Client")
    parser.add_argument("--server", type=str, help="Server Class Name", default = "Markinos_Server")
    parser.add_argument("--packet", type=str, help="Packet Class Name", default = "Markinos_Packet")

    args = parser.parse_args()

    parser = MarkinosParser.MarkinosParser()
    parser.parse_midl(args.path)
   

    #jinja2
    file_loader = jinja2.FileSystemLoader("Templates")
    env = jinja2.Environment(loader = file_loader)
    
    
    template = env.get_template("Markinos_Packet_Template.h")
    output = template.render(parser = parser.proxy_list, packet_name = args.packet)
    
    f = open(args.output + args.packet + ".h", 'w+')
    f.write(output)
    f.close()

    template = env.get_template("Markinos_Packet_Template.cpp")
    output = template.render(parser = parser.proxy_list, client_name = args.client, packet_name = args.packet)
    
    f = open(args.output +  args.packet + ".cpp", 'w+')
    f.write(output)
    f.close()

    template = env.get_template("Markinos_Client_Template.h")
    output = template.render(parser = parser.proxy_list, client_name = args.client, packet_name = args.packet)
    
    f = open(args.output + args.client + ".h", 'w+')
    f.write(output)
    f.close()

  
    template = env.get_template("Markinos_Client_Template.cpp")
    output = template.render(parser = parser.proxy_list, client_name = args.client, packet_name = args.packet)
    
    f = open(args.output + args.client + ".cpp", 'w+')
    f.write(output)
    f.close()

    template = env.get_template("Markinos_Server_Core_Template.h")
    output = template.render(parser = parser.proxy_list, server_name = args.server, packet_name = args.packet)
    
    f = open(args.output + args.server + ".h", 'w+')
    f.write(output)
    f.close()

  
    template = env.get_template("Markinos_Server_Core_Template.cpp")
    output = template.render(parser = parser.proxy_list, server_name = args.server, packet_name = args.packet)
    
    f = open(args.output + args.server + ".cpp", 'w+')
    f.write(output)
    f.close()


    for solo_name, solo_data in parser.solos.items():
         template = env.get_template("Markinos_Server_Solo_Template.h")
         output = template.render(parser = solo_data["functions"], solo_name = solo_name, packet_name = args.packet)
         f = open(args.output + solo_name + ".h", 'w+')
         f.write(output)
         f.close()
   
         template = env.get_template("Markinos_Server_Solo_Template.cpp")
         output = template.render(parser = solo_data["functions"], solo_name = solo_name, packet_name = args.packet)
         f = open(args.output + solo_name + ".cpp", 'w+')
         f.write(output)
         f.close()

    for group_name, group_data in parser.groups.items():
         template = env.get_template("Markinos_Server_Group_Template.h")
         output = template.render(parser = group_data["functions"], group_name = group_name, packet_name = args.packet)
         f = open(args.output + group_name + ".h", 'w+')
         f.write(output)
         f.close()
   
         template = env.get_template("Markinos_Server_Group_Template.cpp")
         output = template.render(parser = group_data["functions"], group_name = group_name, packet_name = args.packet)
         f = open(args.output + group_name + ".cpp", 'w+')
         f.write(output)
         f.close()   

    return

if __name__ == '__main__':
    
    main()
   
