/***************************************************************************
*  File    : SpWrite.c   v1.1                                              *
*  Purpose : puts an image file on a sp disk                               *
*  Language: turbo C 1.0                                                   *
*  Author  : ûincent Raue                                                  *
*  Date    : 15/10/96                                                      *
*  Comment : HD floppy: 2 sides, 80 tracks/side, 18 sectors/track          *
*            512 bytes/sector.                                             *
***************************************************************************/

#include <alloc.h>
#include <dos.h>
#include <fcntl.h>
#include <stat.h>
#include <stdio.h>

main(argc,argv)
   int argc;
   char *argv[];
   {
   char buffer[512*18];
   int drive=0, head, retry=0, spfile, track;
   printf("SpWrite V1.1 by ûincent Raue '96. Puts an image file on a SP disk.\n");
   if(argc==3 && strcmp(argv[2],"b:")==0) drive=1;
   else if(argc!=2)
      {
      printf("Syntax: spwrite <filename.sp> [drive:]\n\n");
      exit(1);
      }
   if((spfile=open(argv[1],O_RDONLY|O_BINARY)) == -1)
      {
      printf("Error: can't open file %s.\n\n",argv[1]);
      exit(1);
      }
   for(track=0;track<=79;track++)
      {
      for(head=0;head<=1;head++)
         {
         if(read(spfile,buffer,512*18) == -1)
            {
            printf("Error: can't read file %s\n\n",argv[1]);
            exit(1);
            }
         /* write sectors */
         printf("Writing track %d\r",track);
         while(biosdisk(3,drive,head,track,1,18,buffer)!=0)
            { /* fdd error */
            retry++;
            if(retry==3)
               {
               printf("Error: can't write floppy.\n\n");
               exit(1);
               }
            else sleep(1);
            }
         }
      }
   close(spfile);
   printf("Ok: %s written to sp disk.\n",argv[1]);
   }

/**************************************************************************/