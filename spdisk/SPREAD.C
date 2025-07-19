/***************************************************************************
*  File    : SpRead.c   v1.1                                               *
*  Purpose : puts an image of a sp disk into a file                        *
*  Language: turbo C 1.0                                                   *
*  Author  : ûincent Raue                                                  *
*  Date    : 15/10/96                                                      *
*  Comment : HD floppy: 2 sides, 80 tracks/side, 18 sectors/track          *
*            512 bytes/sector.                                             *
***************************************************************************/

#include <alloc.h>
#include <bios.h>
#include <fcntl.h>
#include <stat.h>
#include <stdio.h>
#define DEBUG 0

main(argc,argv)
   int argc;
   char *argv[];
   {
   char buffer[512*18];
   int drive=0, error, head, retry=0, spfile, track;
   printf("\nSpRead V1.1 by ûincent Raue '96. Puts an image of a SP disk into a file.\n");
   if(argc==3 && strcmp(argv[2],"b:")==0) drive=1;
   else if(argc!=2)
      {
      printf("Syntax: spread <filename.sp> [drive:]\n\n");
      exit(1);
      }
   if((spfile=open(argv[1],O_CREAT|O_BINARY,S_IWRITE)) == -1)
      {
      printf("Error: can't open file %s\n\n",argv[1]);
      exit(1);
      }
   for(track=0;track<=79;track++)
      {
      for(head=0;head<=1;head++)
         {
         /* read sectors */
         printf("Reading track %d\r",track);
         while((error=biosdisk(2,drive,head,track,1,18,buffer))!=0)
            { /* fdd error */
            if(DEBUG) printf("Error %d while reading track %d. Retrying ...\n",error,track);
            retry++;
            if(retry==3)
               {
               printf("Error: can't read floppy.\n\n");
               unlink(argv[1]);
               exit(1);
               }
            else sleep(1);
            }
         if(write(spfile,buffer,512*18) == -1)
            {
            printf("Error: can't write file %s\n\n",argv[1]);
            exit(1);
            }
         }
      }
   close(spfile);
   printf("Ok: image written to %s\n\n",argv[1]);
   }

/**************************************************************************/