#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <linux/if_vlan.h>
#include <string.h>


#define MAX_HOSTNAME 256


static char* usage = 
      "
Usage: add             [interface-name] [vlan_id]
       rem             [vlan-name]
       set_flag        [interface-name] [flag-num]       [0 | 1]
       set_egress_map  [vlan-name]      [skb_priority]   [vlan_qos]
       set_ingress_map [vlan-name]      [skb_priority]   [vlan_qos]
       set_name_type   [name-type]

* The [interface-name] is the name of the ethernet card that hosts
  the VLAN you are talking about.
* The vlan_id is the identifier (0-4095) of the VLAN you are operating on.
* skb_priority is the priority in the socket buffer (sk_buff).
* vlan_qos is the 3 bit priority in the VLAN header
* name-type:  VLAN_PLUS_VID (vlan0005), VLAN_PLUS_VID_NO_PAD (vlan5),
              DEV_PLUS_VID (eth0.0005), DEV_PLUS_VID_NO_PAD (eth0.5)
* bind-type:  PER_DEVICE  # Allows vlan 5 on eth0 and eth1 to be unique.
              PER_KERNEL  # Forces vlan 5 to be unique across all devices.
* FLAGS:  1 REORDER_HDR  When this is set, the VLAN device will move the
            ethernet header around to make it look exactly like a real
            ethernet device.  This may help programs such as DHCPd which
            read the raw ethernet packet and make assumptions about the
            location of bytes.  If you don't need it, don't turn it on, because
            there will be at least a small performance degradation.  Default
            is OFF.
";

void show_usage() {
   // MATHIEU
   // printf(usage);
   fprintf(stdout,usage);

}

int hex_to_bytes(char* bytes, int bytes_length, char* hex_str) {
   // MATHIEU
   // int hlen = strlen(hex_str);
   int hlen;
   int i;
   
   int j = 0;
   char hex[3];
   char* stop; /* not used for any real purpose */

   // MATHIEU
   hlen = strlen(hex_str);

   hex[2] = 0;

   // MATHIEU
   //for (int i = 0; i<hlen; i++) {
   for (i = 0; i<hlen; i++) {

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

   char* conf_file_name = "/proc/net/vlan/config";

   memset(&if_request, 0, sizeof(struct vlan_ioctl_args));
   
   if ((argc < 3) || (argc > 5)) {
      // MATHIEU
      // cerr << "Expecting argc to be 3-5, inclusive.  Was: " << argc << endl;
      fprintf(stdout,"Expecting argc to be 3-5, inclusive.  Was: %d\n",argc);

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
            // MATHIEU
                    //cerr << "Invalid name type.\n";
            fprintf(stderr,"Invalid name type.\n");
                                 
            show_usage();
            exit(1);
         }
         if_request.u.name_type = nm_type;
      }
      else {
         if_name = argv[2];
         if (strlen(if_name) > 15) {
            // MATHIEU
                //cerr << "ERROR:  if_name must be 15 characters or less." << endl;
            fprintf(stderr,"ERROR:  if_name must be 15 characters or less.\n");
                        
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
      // MATHIEU
      //cerr << "ERROR:  Could not open /proc/vlan/config.\n";
      fprintf(stderr,"ERROR:  Could not open /proc/vlan/config.\n");
          
          exit(3);
   }


   /* add */
   if (strcasecmp(cmd, "add") == 0) {
      if (ioctl(fd, ADD_VLAN_IOCTL, &if_request) < 0) {
         // MATHIEU
             //cerr << "ERROR: trying to add VLAN #" << vid << " to IF -:"
         //     << if_name << ":-  error: " << strerror(errno) << endl;
         fprintf(stderr,"ERROR: trying to add VLAN #%u to IF -:%s:-  error: %s\n",
                    vid, if_name, strerror(errno));
                 
      }
      else {
         // MATHIEU
         // cout << "Added VLAN with VID == " << vid << " to IF -:"
         //     << if_name << ":-" << endl;
         fprintf(stdout,"Added VLAN with VID == %u to IF -:%s:-\n",
                            vid, if_name);
         if (vid == 1) {
            fprintf(stdout, "WARNING:  VLAN 1 does not work with many switches,\nconsider another number if you have problems.\n");
         }
      }
   }//if
   else if (strcasecmp(cmd, "rem") == 0) {
      if (ioctl(fd, DEL_VLAN_IOCTL, &if_request) < 0) {
         // MATHIEU
                 //cerr << "ERROR: trying to remove VLAN #" << vid << " to IF -:"
         //     << if_name << ":-  error: " << strerror(errno) << endl;
         fprintf(stderr,"ERROR: trying to remove VLAN -:%s:- error: %s\n",
                 if_name, strerror(errno));
         
      }
      else {
             // MATHIEU
         //cout << "Removed VLAN with VID == " << vid << " from IF -:"
                 //     << if_name << ":-" << endl;
         fprintf(stdout,"Removed VLAN -:%s:-\n", if_name);

      }
   }//if
   else if (strcasecmp(cmd, "set_egress_map") == 0) {
      if (ioctl(fd, SET_EGRESS_PRIORITY_IOCTL, &if_request) < 0) {
         // MATHIEU
         //cerr << "ERROR: trying to set egress map on device -:"
         //     << if_name << ":-  error: " << strerror(errno) << endl;
         fprintf(stderr,"ERROR: trying to set egress map on device -:%s:- error: %s\n",
                                if_name, strerror(errno));
         
      }
      else {
         // MATHIEU
         //cout << "Set egress mapping on device -:"
         //     << if_name << ":-  Should be visible in /proc/net/vlan/" << if_name << endl;
         fprintf(stdout,"Set egress mapping on device -:%s:- "
                                "Should be visible in /proc/net/vlan/%s\n",
                                if_name, if_name);
         
      }
   }
   else if (strcasecmp(cmd, "set_ingress_map") == 0) {
      if (ioctl(fd, SET_INGRESS_PRIORITY_IOCTL, &if_request) < 0) {
         // MATHIEU
         //cerr << "ERROR: trying to set ingress map on device -:"
         //     << if_name << ":-  error: " << strerror(errno) << endl;
         fprintf(stderr,"ERROR: trying to set ingress map on device -:%s:- error: %s\n",
                                if_name, strerror(errno));
         
      }
      else {
         // MATHIEU
         //cout << "Set ingress mapping on device -:"
         //     << if_name << ":-  Should be visible in /proc/net/vlan/" << if_name << endl;
         fprintf(stdout,"Set ingress mapping on device -:%s:- "
                                "Should be visible in /proc/net/vlan/%s\n",
                                if_name, if_name);
                                 
      }
   }   
   else if (strcasecmp(cmd, "set_flag") == 0) {
      if (ioctl(fd, SET_VLAN_FLAG_IOCTL, &if_request) < 0) {
         // MATHIEU
         //cerr << "ERROR: trying to set flag on device -:"
         //     << if_name << ":-  error: " << strerror(errno) << endl;
         fprintf(stderr,"ERROR: trying to set flag on device -:%s:- error: %s\n",
                            if_name, strerror(errno));
         
      }
      else {
         // MATHIEU
         //cout << "Set flag on device -:"
         //     << if_name << ":-  Should be visible in /proc/net/vlan/" << if_name << endl;
         fprintf(stdout,"Set flag on device -:%s:- "
                            "Should be visible in /proc/net/vlan/%s\n",
                                if_name, if_name);
         
      }
   }   
   else if (strcasecmp(cmd, "set_name_type") == 0) {
      if (ioctl(fd, SET_NAME_TYPE_IOCTL, &if_request) < 0) {
         // MATHIEU
         //cerr << "ERROR: trying to set name type for VLAN subsystem, error: "
         //     << strerror(errno) << endl;
         fprintf(stderr,"ERROR: trying to set name type for VLAN subsystem, error: %s\n",
                                strerror(errno));
         
      }
      else {
         // MATHIEU
         //cout << "Set name-type for VLAN subsystem.  Should be visible in /proc/net/vlan/config"
         //     << endl;
         fprintf(stdout,"Set name-type for VLAN subsystem."
                                " Should be visible in /proc/net/vlan/config\n");
         
      }
   }
   else {
      // MATHIEU
      //cerr << "Unknown command -:" << cmd << ":-\n";
      fprintf(stderr,"Unknown command -:%s:-\n", cmd);

      show_usage();
      exit(5);
   }

   return 0;
}/* main */
