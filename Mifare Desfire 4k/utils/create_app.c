#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <nfc/nfc.h>
#include <openssl/des.h>
#include <string.h>

#define MAX_FRAME_LEN 264
#define DEBUG
#define MAX_APPLICATIONS 26

/*APDU Declaration*/
/*zero key*/
uint8_t defaultkey[8]             = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

/*test challenge*/
uint8_t datatest[8]               = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};

uint8_t createApplication_apdu[11] = {0x90,0xCA,0x00,0x00,0x05,0x00,0x00,0x00,0x0f,0x01,0x00};
uint8_t deleteApplication_apdu[9] = {0x90,0xDA,0x00,0x00,0x03,0x00,0x00,0x00,0x00};
uint8_t list1Application_apdu[5] = {0x90,0x6A,0x00,0x00,0x00};
uint8_t list2Application_apdu[5] = {0x90,0xAF,0x00,0x00,0x00};
uint8_t authenticationStep1_apdu[7] = {0x90,0x0A,0x00,0x00,0x01,0x00,0x00};
uint8_t authenticationStep2_apdu[22] = {0x90,0xAF,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

/*LibNFC global variable*/
nfc_context *context = NULL;    /*context pointer          */
nfc_device  *pnd     = NULL;    /*device pointer           */

/*utils command prototypes*/
void onExit          (void);
static void print_hex(const uint8_t *pbtData, const size_t szBytes);
void init            (void);


int createApplication(unsigned int AID);
int deleteApplication(unsigned int AID);
int listApplication(unsigned int *outputList, int *outputCount);
int authentification(uint8_t key_index,  uint8_t * key1, uint8_t * key2, uint8_t * challenge);
int  isValidPrim     (uint8_t * noPrim, uint8_t * Prim);
void xor             (uint8_t * input1, uint8_t * input2, uint8_t * output);
void buildPrim       (uint8_t * data);
void encrypt         (uint8_t * input, uint8_t * output, uint8_t *key1, uint8_t *key2);
int sendRequest2(uint8_t * abtTx, size_t abtTx_size, uint8_t * abtRx, int expected_status_word, int alternative_status_word, int *data_length);
void usage(char *progname);


/**
* The main method
*
* @param argc, the number of element in the array argv
* @param argv, the list of argument of this program.  The first item is the program name
* @return EXIT_FAILURE if an error occurs, EXIT_SUCCESS otherwise
*
*/
int main(int argc, char *argv[])
{


  int i, length, ch;
  unsigned int *test;
  length =0;
  if(argc<2){
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  while ((ch = getopt (argc, argv, "c:d:l")) != -1) {
    switch (ch) {
      case 'c':
        init();
        if(createApplication(atoi(optarg)) != 0)
        {
          printf("Failure\n");
          return EXIT_FAILURE;
        }
        break;
      case 'd':
        init();
        if(deleteApplication(atoi(optarg)) != 0)
        {
            printf("Failure\n");
            return EXIT_FAILURE;
        }
        break;
      case 'l':
        init();
        if(listApplication(test,&length) != 0)
        {
          printf("Failure\n");
          return EXIT_FAILURE;
        }
        printf("size:%d\n",length);
        fflush(stdout);
        for(i=0;i<length;i++){
          //printf("AIDS : %d\n", test[i]);
        }
        break;
      default:
        break;
      }
    }


      printf("Success\n");

      return EXIT_SUCCESS;
    }


    void usage(char *progname)
    {
        fprintf (stderr, "usage: %s [-c:d:l]\n", progname);
        fprintf (stderr, "\nOptions:\n");
        fprintf (stderr, "  -c n: Create application with n as AID\n");
        fprintf (stderr, "  -d n: Delete application with n as AID\n");
        fprintf (stderr, "  -l: Lists all applications in the card\n");
    }

    /**
    * this function create a new application
    *
    * @param AID, the application identifier to create, it must be a value between 0x000001 and 0xffffff
    * @return the status word, or -1 if an error has occured
    */
    int createApplication(unsigned int AID)
    {
      uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
      uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
      size_t abtTx_size;

      /*check the AID*/
      if(AID > 0xffffff || AID < 0x000001)
        {
          fprintf(stderr, "Invalid AID, expected a value between 0x000001 and 0xffffff, got 0x%06x\n",AID);
          return -1;
        }

        /*prepare the data*/
        abtTx_size = 11;
        memcpy(abtTx,createApplication_apdu,abtTx_size);
        abtTx[7] = (AID >> 16) & 0xff; /*data 0*/
        abtTx[6] = (AID >> 8 ) & 0xff; /*data 1*/
        abtTx[5] = AID & 0xff;         /*data 2*/

        #ifdef DEBUG
        /*debug message*/
        printf("Create Application 0x%06x\n",AID);
        #endif

        /*send the data to the card, the expected status word is 0x91 0x00*/
        if(sendRequest(abtTx, abtTx_size, abtRx, 0x9100, 0) != 0) {return -1;}

          return 0;
        }

        /**
        * this function deletes an application
        *
        * @param AID, the application identifier to delete, it must be a value between 0x000001 and 0xffffff
        * @return the status word, or -1 if an error has occured
        */
        int deleteApplication(unsigned int AID)
        {
          uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
          uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
          size_t abtTx_size;

          authentification(0, defaultkey, defaultkey, datatest);

          /*check the AID*/
          if(AID > 0xffffff || AID < 0x000001)
            {
              fprintf(stderr, "Invalid AID, expected a value between 0x000001 and 0xffffff, got 0x%06x\n",AID);
              return -1;
            }

            /*prepare the data*/
            abtTx_size = 9;
            memcpy(abtTx,deleteApplication_apdu,abtTx_size);
            abtTx[7] = (AID >> 16) & 0xff; /*data 0*/
            abtTx[6] = (AID >> 8 ) & 0xff; /*data 1*/
            abtTx[5] = AID & 0xff;         /*data 2*/

            #ifdef DEBUG
            /*debug message*/
            printf("Delete Application 0x%06x\n",AID);
            #endif

            /*send the data to the card, the expected status word is 0x91 0x00*/
            if(sendRequest(abtTx, abtTx_size, abtRx, 0x9100, 0) != 0) {return -1;}

              return 0;
            }

            /**
            * this function lists all the applications
            *
            * @param outputList, a pointer to a list of integer to fill, the list must have at least 28 element
            * @parem outputCount, a pointer to an integer, this integer will contain the number of application
            * @return the status word, or -1 if an error has occured
            */
            int listApplication(unsigned int *outputList, int *outputCount)
            {
              uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
              uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
              size_t abtTx_size;
              int more, oldLenght;
              int length = 0;


              //authentification(0, defaultkey, defaultkey, datatest);


              /*prepare the data*/
              abtTx_size = 5;
              memcpy(abtTx,list1Application_apdu,abtTx_size);

              #ifdef DEBUG
              /*debug message*/
              printf("List Applications\n");
              #endif

              /*send the data to the card, the expected status word is 0x91 0x00 if no more data is available*/

              more = sendRequest2(abtTx, abtTx_size, abtRx, 0x9100, 0x91AF, &length);
              printf("Response %x %02x Size:%d\n", abtRx[length-2],abtRx[length-1], length);
              int i,AID;
              unsigned int result[length];
              for(i=0;i<length-2;){
                AID = (abtRx[i+2]>>16) + (abtRx[i+1]>>8) + (abtRx[i]);
                printf("AID : %d\n",AID);
                result[i] = AID;
                i= i+3;
              }

              if(abtRx[length-1] == 0xAF){ // fetch more data
                memcpy(abtTx,list2Application_apdu,abtTx_size);
                  return -1;
                oldLenght = length;
                if(sendRequest2(abtTx, abtTx_size, abtRx, 0x9100, 0x9100, &length)!=0){
                }else{
                  unsigned int moreResults[oldLenght+length];
                  memcpy(moreResults, result, oldLenght);
                  for(i=0;i<length-2;){
                    AID = (abtRx[i+2]>>16) + (abtRx[i+1]>>8) + (abtRx[i]);
                    printf("AID : %d\n",AID);
                    moreResults[oldLenght + i] = AID;
                    i= i+3;
                  }
                  length= length+oldLenght;
                  outputList = &moreResults[0];
                  memcpy(outputCount, &length, sizeof(int));
                }
              }else{ // set outputlist
                outputList= result;
                memcpy(outputCount, &length, sizeof(int));
                fflush(stdout);
              }



              return 0;
            }

            /**
            * this function makes an authentication with the desfire tag
            *
            * @param key_index, the key index on which the authentication must occur
            * @param key1, the first 8-bytes of the 16-bytes key
            * @param key2, the second 8-bytes of the 16-bytes key, if the key has 8-bytes, the key2 must be the same as key1
            * @param challenge, a 8-bytes array, it is the reader challenge to send to the desfire
            * @return 0 if the authentication is successfull, -1 otherwise
            *
            * WARNING, if the authentication key is incorrect, the error message will be the following :
            *     "Invalid status word on authentication pass 2, expected 0x9100 got 0x91ae"
            *
            */
            int authentification(uint8_t key_index,  uint8_t * key1, uint8_t * key2, uint8_t * challenge)
            {
              int res, status_word;

              uint8_t abtRx[MAX_FRAME_LEN];  /*output buffer         */
              uint8_t abtTx[MAX_FRAME_LEN];  /*input buffer          */
              size_t abtTx_size;

              uint8_t output[8], output2[8]; /*temporary buffer      */

              /*check the args*/
              if(key_index > 0xD)
                {
                  fprintf(stderr, "Invalid key index, expected a value between 0x0 and 0xd, got 0x%01x\n",key_index);
                  return -1;
                }

                /*prepare the data of the pass 1*/
                abtTx_size = 7;
                memcpy(abtTx,authenticationStep1_apdu,abtTx_size);
                abtTx[5] = key_index & 0xff; /*data 0*/

                #ifdef DEBUG
                printf("Authentication on key 0x%02x, pass 1\n",key_index);
                #endif

                /*send the request to the card*/
                /*the status word 0x91AF is expected, it means "ADDITIONAL FRAME".  The desfire tag is waiting the second pass of the authentication*/
                /*8-bytes are expected, it is the tag challenge*/
                if(sendRequest(abtTx, abtTx_size, abtRx, 0x91AF, 8) != 0) {return -1;}

                  /*a) get the challenge plain text*/
                  encrypt(abtRx, output, key1,key2);
                  #ifdef DEBUG
                  printf("\nnt: ");print_hex(output,8);
                  #endif

                  /*transform the tag challenge into a prim challenge*/
                  /*the first byte juste go at the end of the array*/
                  buildPrim(output);
                  #ifdef DEBUG
                  printf("nt': ");print_hex(output,8);
                  #endif

                  /*b) encrypt the reader challenge*/
                  encrypt(challenge, output2, key1,key2);

                  #ifdef DEBUG
                  printf("nr: ");print_hex(challenge,8);
                  printf("D1: ");print_hex(output2,8);
                  #endif

                  /*c) xor the cyphered challenge of the reader with the prim challenge of the tag*/
                  xor(output2,output, output);
                  #ifdef DEBUG
                  printf("Buffer: ");print_hex(output,8);
                  #endif

                  /*d) cypher the result of the step c)*/
                  encrypt(output, output, key1,key2);
                  #ifdef DEBUG
                  printf("D2: ");print_hex(output,8);printf("\n");
                  #endif

                  /*e) prepare the data of the pass 2*/
                  abtTx_size = 22;
                  memcpy(abtTx,authenticationStep2_apdu,abtTx_size);
                  memcpy(&(abtTx[5]), output2, 8);
                  memcpy(&(abtTx[13]), output, 8);

                  #ifdef DEBUG
                  printf("Authentication on key 0x%02x, pass 2\n",key_index);
                  #endif

                  /*send the authentication pass 2*/
                  if(sendRequest(abtTx, abtTx_size, abtRx, 0x9100, 8) != 0) {return -1;}

                    /*get the plain text of the tag response*/
                    encrypt(abtRx, output, key1,key2);
                    #ifdef DEBUG
                    printf("\nnr': ");print_hex(output,8);
                    #endif

                    /*check the tag answer*/
                    return isValidPrim(challenge, output);
                  }


                  /**
                  * This function compares two 8-bytes array to check if second array is a Prim version of the first array
                  *
                  * @param noPrim, the no prim version of the 8-bytes array
                  * @param Prim, the prim version of the 8-bytes array
                  * @return 0 if Prim is the prim version of noPrim, -1 otherwise
                  *
                  * Example: [2,3,4,5,6,7,8,1] is the prim version of the following array [1,2,3,4,5,6,7,8]
                  *
                  */
                  int isValidPrim(uint8_t * noPrim, uint8_t * Prim)
                  {
                    int iterator;

                    for(iterator = 0; iterator <8;iterator+=1)
                      {
                        if(noPrim[ (iterator+1)%8] != Prim[ iterator ])
                          return -1;
                        }

                        return 0;
                      }

                      /**
                      * this procedure xor two 8-bytes arrays and put the result in a third array
                      *
                      * @param input1, it is the first 8-bytes array to xor
                      * @param input2, it is the second 8-bytes array to xor
                      * @param output, it is an 8-bytes array to put the result
                      *
                      */
                      void xor(uint8_t * input1, uint8_t * input2, uint8_t * output)
                      {
                        int iterator;

                        for(iterator = 0; iterator <8;iterator+=1)
                          {
                            output[iterator] = input1[iterator] ^ input2[iterator];
                          }
                        }

                        /**
                        * this procedure convert a 8-bytes array into its Prim image
                        *
                        * @param data, the 8-bytes array to convert
                        *
                        */
                        void buildPrim(uint8_t * data)
                        {
                          uint8_t tmp;
                          int iterator;

                          tmp = data[0];

                          for(iterator = 1; iterator <8;iterator+=1)
                            {
                              data[iterator-1] = data[iterator];
                            }

                            data[7] = tmp;
                          }

                          /**
                          * This procedure decypher the input data.  The data is not cyphered because the desfire is only able to cypher the data.
                          * So, to send protected data to the desfire tag, it must be decyphered.
                          * the desfire use 3DES ECB EDE with two 8-bytes keys
                          *
                          * @param input, the 8-bytes array to decypher
                          * @param output, the 8-bytes array to put the result
                          * @param key1, the first DES key
                          * @param key2, the second DES key
                          *
                          */
                          void encrypt(uint8_t * input, uint8_t * output, uint8_t *key1, uint8_t *key2)
                          {
                            DES_key_schedule ks1, ks2;

                            /*set keys*/
                            DES_set_key_unchecked((DES_cblock*)key1,&ks1);
                            DES_set_key_unchecked((DES_cblock*)key2,&ks2);

                            /*encrypt*/
                            DES_ecb2_encrypt((DES_cblock*)input, (DES_cblock*)output,&ks1,&ks2, DES_DECRYPT);
                          }


                          /**
                          * this function sends a command to a desfire tag, check the status word and the size of the output data
                          *
                          * @param abtTx, the command to send to the tag
                          * @param abtTx_size, the size of the command
                          * @param abtRx, the buffer to store the answer of the tag
                          * @param expected_status_word, the expected status word
                          * @param expected_data_length, the expected data length
                          * @return -1 if an error occurs, 0 otherwise
                          *
                          * WARNING, this function exit the program if there is a communication error with the reader or with the desfire tag
                          *
                          */
                          int sendRequest(uint8_t * abtTx, size_t abtTx_size, uint8_t * abtRx, int expected_status_word, int expected_data_length)
                          {
                            int res, status_word;

                            #ifdef DEBUG
                            printf("tx: ");
                            print_hex(abtTx,abtTx_size);
                            #endif

                            /*send the request*/
                            res = nfc_initiator_transceive_bytes(pnd, abtTx, abtTx_size, abtRx, MAX_FRAME_LEN, 0);
                            if(res < 0)
                              {
                                nfc_perror(pnd, "nfc_initiator_transceive_bytes");
                                exit(EXIT_FAILURE);
                              }

                              #ifdef DEBUG
                              printf("rx: ");
                              print_hex(abtRx,res);
                              #endif

                              /*check the status word*/
                              if(res > 1)
                                {
                                  /*the status word is build with the two last bytes of the output*/
                                  status_word = abtRx[res-2];
                                  status_word = (status_word<<8) + abtRx[res-1];

                                  if(status_word != expected_status_word)
                                    {
                                      fprintf(stderr, "Invalid status word on request sending, expected 0x%04x got 0x%04x\n",expected_status_word,status_word);
                                      return -1;
                                    }
                                  }
                                  else
                                    {
                                      fprintf(stderr, "Invalid response length on request sending, expected 10 bytes got %d\n",res);
                                      return -1;
                                    }

                                    /*check the data length*/
                                    if((res-2) != expected_data_length) /*check the data length*/
                                      {
                                        fprintf(stderr, "Invalid length of the response data on request sending, expected %d bytes got %d\n", expected_data_length,res-2);
                                        return -1;
                                      }

                                      return 0;
                                    }

                                    int sendRequest2(uint8_t * abtTx, size_t abtTx_size, uint8_t * abtRx, int expected_status_word, int alternative_status_word, int *data_length)
                                    {
                                      int res, status_word;

                                      #ifdef DEBUG
                                      printf("tx: ");
                                      print_hex(abtTx,abtTx_size);
                                      #endif

                                      /*send the request*/
                                      res = nfc_initiator_transceive_bytes(pnd, abtTx, abtTx_size, abtRx, MAX_FRAME_LEN, 0);
                                      if(res < 0)
                                        {
                                          nfc_perror(pnd, "nfc_initiator_transceive_bytes");
                                          exit(EXIT_FAILURE);
                                        }

                                        #ifdef DEBUG
                                        printf("rx: ");
                                        print_hex(abtRx,res);
                                        #endif

                                        /*check the status word*/
                                        if(res > 1)
                                          {
                                            /*the status word is build with the two last bytes of the output*/
                                            status_word = abtRx[res-2];
                                            status_word = (status_word<<8) + abtRx[res-1];

                                            if(status_word != expected_status_word && status_word != alternative_status_word)
                                              {
                                                fprintf(stderr, "Invalid status word on request sending, expected 0x%04x got 0x%04x\n",expected_status_word,status_word);
                                                return -1;
                                              }
                                            }
                                            else
                                              {
                                                fprintf(stderr, "Invalid response length on request sending, expected 10 bytes got %d\n",res);
                                                return -1;
                                              }

                                              /*check the data length*/
                                              memcpy(data_length,&res,sizeof(int));
                                              return 0;
                                            }

                                            /**
                                            * this procedure initialize the lib libnfc and poll a card
                                            */
                                            void init(void)
                                            {
                                              nfc_target nt;                  /*target value           */
                                              const nfc_modulation nmMifare = /*communication settings */
                                              {
                                                .nmt = NMT_ISO14443A, /*communication standard       */ /*other value : NMT_ISO14443A, NMT_JEWEL, NMT_ISO14443B, NMT_ISO14443BI, NMT_ISO14443B2SR, NMT_ISO14443B2CT, NMT_FELICA, NMT_DEP*/
                                                .nbr = NBR_106,       /*communication speed 106kb/s  */ /*other value : NBR_UNDEFINED, NBR_106, NBR_212, NBR_424, NBR_847*/
                                              };

                                              /*define the atexit procedure*/
                                              if (atexit(onExit) != 0)
                                                {
                                                  fprintf(stderr, "cannot set exit function\n");
                                                  exit(EXIT_FAILURE);
                                                }

                                                /*context establishment*/
                                                nfc_init(&context);
                                                if (context == NULL)
                                                  {
                                                    fprintf(stderr,"Unable to init libnfc (malloc)\n");
                                                    exit(EXIT_FAILURE);
                                                  }

                                                  /*print the lib version*/
                                                  printf("libnfc %s\n", nfc_version());

                                                  /*open the first device available*/
                                                  if ((pnd = nfc_open(context, NULL)) == NULL)
                                                    {
                                                      fprintf(stderr, "%s", "Unable to open NFC device. (don't forget to be root)\n");
                                                      exit(EXIT_FAILURE);
                                                    }

                                                    printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

                                                    /*configure the device in reader mode*/
                                                    if (nfc_initiator_init(pnd) < 0)
                                                      {
                                                        nfc_perror(pnd, "nfc_initiator_init");
                                                        exit(EXIT_FAILURE);
                                                      }
                                                      printf("initiator mode: ENABLE\n");

                                                      /*select target, poll a tag and print the information*/
                                                      if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) > 0)
                                                        {
                                                          printf("The following (NFC) ISO14443A tag was found:\n");

                                                          printf("    ATQA (SENS_RES): ");
                                                          print_hex(nt.nti.nai.abtAtqa, 2);

                                                          printf("       UID (NFCID%c): ", (nt.nti.nai.abtUid[0] == 0x08 ? '3' : '1'));
                                                          print_hex(nt.nti.nai.abtUid, nt.nti.nai.szUidLen);

                                                          printf("      SAK (SEL_RES): ");
                                                          print_hex(&nt.nti.nai.btSak, 1);

                                                          if (nt.nti.nai.szAtsLen)
                                                            {
                                                              printf("          ATS (ATR): ");
                                                              print_hex(nt.nti.nai.abtAts, nt.nti.nai.szAtsLen);
                                                            }
                                                          }
                                                        }


                                                        /**
                                                        * this procedure convert an array of bytes in string and print it on the standard output
                                                        *
                                                        * @param pbtData, the array of bytes to print
                                                        * @param pbtData, the size of the array of bytes
                                                        *
                                                        */
                                                        static void print_hex(const uint8_t *pbtData, const size_t szBytes)
                                                        {
                                                          size_t  szPos;

                                                          for (szPos = 0; szPos < szBytes; szPos++)
                                                            {
                                                              printf("%02x  ", pbtData[szPos]);
                                                            }
                                                            printf("\n");
                                                          }

                                                          /**
                                                          * this procedure release the LibNFC ressources on the exit event
                                                          */
                                                          void onExit(void)
                                                          {
                                                            /*device disconnection*/
                                                            if(pnd != NULL){nfc_close(pnd);}

                                                              /*context destruction*/
                                                              if(context != NULL){nfc_exit(context);}
                                                              }