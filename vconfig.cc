#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream.h>
#include <fstream.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <linux/if_vlan.h>
#include <string.h>


#define MAX_HOSTNAME 256

// END   Copied from linux/net/802_1Q/vlanproc.h --BLG


static char* usage =
      "
Usage: add             [interface-name] [vlan_id]
       rem             [vlan-name]
       set_dflt        [interface-name] [vlan_id]
       add_port        [port-name]      [vlan_id]
       rem_port        [port-name]      [vlan_id]
       set_egress_map  [vlan-name]      [skb_priority]   [vlan_qos]
       set_ingress_map [vlan-name]      [skb_priority]   [vlan_qos]
       set_name_type   [name-type]
       set_bind_mode   [bind-type]

* The [interface-name] is the name of the ethernet card that hosts
  the VLAN you are talking about.
* The port-name is the name of the physical interface that a VLAN
  may be attached to.
* The vlan_id is the identifier (0-4095) of the VLAN you are operating on.
* skb_priority is the priority in the socket buffer (sk_buff).
* vlan_qos is the 3 bit priority in the VLAN header
* name-type:  VLAN_PLUS_VID (vlan0005), VLAN_PLUS_VID_NO_PAD (vlan5),
              DEV_PLUS_VID (eth0.0005), DEV_PLUS_VID_NO_PAD (eth0.5)
* bind-type:  PER_DEVICE  # Allows vlan 5 on eth0 and eth1 to be unique.
              PER_KERNEL  # Forces vlan 5 to be unique across all devices.

";

void show_usage() {
   printf(usage);
}

int hex_to_bytes(char* bytes, int bytes_length, char* hex_str) {
   int hlen = strlen(hex_str);
   int j = 0;
   char hex[3];
   char* stop; /* not used for any real purpose */

   hex[2] = 0;
   for (int i = 0; i<hlen; i++) {
      hex[0] = hex_str[i];
      i++;
      if (i >= hlen) {
         return j; /* done */
      }
      
      hex[1] = hex_str[i];
      bytes[j++] = (char)strtoul(hex, &stop, 16);
   }
   return j;
}


int main(int argc, char** argv) {
   int fd;
   struct vlan_ioctl_args if_request;
   
   char* cmd = NULL;
   char* if_name = NULL;
   unsigned int vid = 0;
   unsigned int skb_priority;
   unsigned short vlan_qos;
   unsigned int nm_type = VLAN_NAME_TYPE_PLUS_VID;
   unsigned int bind_type = VLAN_BIND_PER_KERNEL;

   char* conf_file_name = "/proc/net/vlan/config";

   memset(&if_request, 0, sizeof(struct vlan_ioctl_args));
   
   if ((argc < 3) || (argc > 5)) {
      cerr << "Expecting argc to be 3-5, inclusive.  Was: " << argc << endl;
      show_usage();
      exit(1);
   }
   else {
      cmd = argv[1];

      if (strcasecmp(cmd, "set_name_type") == 0) {
         if (strcasecmp(argv[2], "VLAN_PLUS_VID") == 0) {
            nm_type = VLAN_NAME_TYPE_PLUS_VID;
         }
         else if (strcasecmp(argv[2], "VLAN_PLUS_VID_NO_PAD") == 0) {
            nm_type = VLAN_NAME_TYPE_PLUS_VID_NO_PAD;
         }
         else if (strcasecmp(argv[2], "DEV_PLUS_VID") == 0) {
            nm_type = VLAN_NAME_TYPE_RAW_PLUS_VID;
         }
         else if (strcasecmp(argv[2], "DEV_PLUS_VID_NO_PAD") == 0) {
            nm_type = VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD;
         }
         else {
            cerr << "Invalid name type.\n";
            show_usage();
            exit(1);
         }
         if_request.u.name_type = nm_type;
      }
      else if (strcasecmp(cmd, "set_bind_mode") == 0) {
         if (strcasecmp(argv[2], "PER_DEVICE") == 0) {
            bind_type = VLAN_BIND_PER_INTERFACE;
         }
         else if (strcasecmp(argv[2], "PER_KERNEL") == 0) {
            bind_type = VLAN_BIND_PER_KERNEL;
         }
         else {
            cerr << "Invalid bind type.\n";
            show_usage();
            exit(1);
         }
         if_request.u.bind_type = bind_type;
      }
      else {
         if_name = argv[2];
         if (strlen(if_name) > 15) {
            cerr << "ERROR:  if_name must be 15 characters or less." << endl;
            exit(1);
         }
         strcpy(if_request.dev1, if_name);
      }

      if (argc == 4) {
         vid = atoi(argv[3]);
         if_request.u.VID = vid;
      }

      if (argc == 5) {
         skb_priority = atoi(argv[3]);
         vlan_qos = atoi(argv[4]);
         if_request.u.skb_priority = skb_priority;
         if_request.vlan_qos = vlan_qos;
      }
   }
   
   // Open up the /proc/vlan/config
   if ((fd = open(conf_file_name, O_RDONLY)) < 0) {
      cerr << "ERROR:  Could not open /proc/vlan/config.\n";
      exit(3);
   }


   /* add */
   if (strcasecmp(cmd, "add") == 0) {
      if (ioctl(fd, ADD_VLAN_IOCTL, &if_request) < 0) {
         cerr << "ERROR: trying to add VLAN #" << vid << " to IF -:"
              << if_name << ":-  error: " << strerror(errno) << endl;
      }
      else {
         cout << "Added VLAN with VID == " << vid << " to IF -:"
              << if_name << ":-" << endl;
      }
   }//if
   else if (strcasecmp(cmd, "rem") == 0) {
      if (ioctl(fd, DEL_VLAN_IOCTL, &if_request) < 0) {
         cerr << "ERROR: trying to remove VLAN #" << vid << " to IF -:"
              << if_name << ":-  error: " << strerror(errno) << endl;
      }
      else {
         cout << "Removed VLAN with VID == " << vid << " from IF -:"
              << if_name << ":-" << endl;
      }
   }//if
   else if (strcasecmp(cmd, "set_dflt") == 0) {
      if (ioctl(fd, SET_DEFAULT_VLAN_ID_IOCTL, &if_request) < 0) {
         cerr << "ERROR: trying to set default VID #" << vid << " on IF -:"
              << if_name << ":-  error: " << strerror(errno) << endl;
      }
      else {
         cout << "Set default VID #" << vid << " on IF -:"
              << if_name << ":-" << endl;
      }
   }//if
   else if (strcasecmp(cmd, "add_port") == 0) {
      if (ioctl(fd, ADD_VLAN_TO_PORT_IOCTL, &if_request) < 0) {
         cerr << "ERROR: trying to add VLAN #" << vid << " to port -:"
              << if_name << ":-  error: " << strerror(errno) << endl;
      }
      else {
         cout << "Added VID #" << vid << " to port -:"
              << if_name << ":-" << endl;
      }
   }//if
   else if (strcasecmp(cmd, "rem_port") == 0) {
      if (ioctl(fd, REM_VLAN_FROM_PORT_IOCTL, &if_request) < 0) {
         cerr << "ERROR: trying to remove VLAN #" << vid << " from port -:"
              << if_name << ":-  error: " << strerror(errno) << endl;
      }
      else {
         cout << "Removed VID #" << vid << " from port -:"
              << if_name << ":-" << endl;
      }
   }//if
   else if (strcasecmp(cmd, "set_egress_map") == 0) {
      if (ioctl(fd, SET_EGRESS_PRIORITY_IOCTL, &if_request) < 0) {
         cerr << "ERROR: trying to set egress map on device -:"
              << if_name << ":-  error: " << strerror(errno) << endl;
      }
      else {
         cout << "Set egress mapping on device -:"
              << if_name << ":-  Should be visible in /proc/net/vlan/" << if_name << endl;
      }
   }   
   else if (strcasecmp(cmd, "set_ingress_map") == 0) {
      if (ioctl(fd, SET_INGRESS_PRIORITY_IOCTL, &if_request) < 0) {
         cerr << "ERROR: trying to set ingress map on device -:"
              << if_name << ":-  error: " << strerror(errno) << endl;
      }
      else {
         cout << "Set ingress mapping on device -:"
              << if_name << ":-  Should be visible in /proc/net/vlan/" << if_name << endl;
      }
   }   
   else if (strcasecmp(cmd, "set_name_type") == 0) {
      if (ioctl(fd, SET_NAME_TYPE_IOCTL, &if_request) < 0) {
         cerr << "ERROR: trying to set name type for VLAN subsystem, error: "
              << strerror(errno) << endl;
      }
      else {
         cout << "Set name-type for VLAN subsystem.  Should be visible in /proc/net/vlan/config"
              << endl;
      }
   }
   else if (strcasecmp(cmd, "set_bind_mode") == 0) {
      if (ioctl(fd, SET_BIND_TYPE_IOCTL, &if_request) < 0) {
         cerr << "ERROR: trying to set bind type for VLAN subsystem. error: "
              << strerror(errno) << endl;
      }
      else {
         cout << "Set bind-type for VLAN subsytem.  Should be visible in /proc/net/vlan/config"
              << endl;
      }
   }   
   else {
      cerr << "Unknown command -:" << cmd << ":-\n";
      show_usage();
      exit(5);
   }

}/* main */
