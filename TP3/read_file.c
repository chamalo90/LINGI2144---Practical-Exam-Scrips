#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <nfc/nfc.h>
#include <openssl/des.h>
#include <string.h>

#define MAX_FRAME_LEN 264
#define DEBUG

/*APDU Declaration*/
uint8_t deleteFile_apdu[7] = {0x90,0xDF,0x00,0x00,0x01,0x00,0x00};
uint8_t createFile_apdu[13] = {0x90,0xCD,0x00,0x00,0x07,0x00,0x00,0xEE,0xEE,0x00,0x00,0x00,0x00}; // 7bytes of data from [5] to [11]
uint8_t listFile_apdu[5] = {0x90,0x6F,0x00,0x00,0x00};
uint8_t readFile_apdu[13] = {0x90,0xBD,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; 
uint8_t readFile2_apdu[5] = {0x90,0xAF,0x00,0x00,0x00};
uint8_t writeFile_apdu[4] = {0x90,0x3D,0x00,0x00}; 
uint8_t writeFile2_apdu[4] = {0x90,0xAF,0x00,0x00};

uint8_t selectApplication_apdu[9] = {0x90,0x5A,0x00,0x00,0x03,0x00,0x00,0x00,0x00};


/*zero key*/
uint8_t defaultkey[8]        = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

/*test challenge*/
uint8_t datatest[8]          = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
uint8_t auth_pass1[7]        = {0x90,0x0A,0x00,0x00,0x01,0x00,0x00};
uint8_t auth_pass2[22]       = {0x90,0xAF,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

/*LibNFC global variable*/
nfc_context *context = NULL;    /*context pointer          */
nfc_device  *pnd     = NULL;    /*device pointer           */

/*utils command prototypes*/
void onExit          (void);
static void print_hex(const uint8_t *pbtData, const size_t szBytes);
void init            (void);


int createFile(unsigned int FID);
int deleteFile(unsigned int FID);
int listFile(unsigned int *outputList, int *outputCount);
int readFile(unsigned int  FID, uint8_t *buf, int nbyte);
int writeFile(unsigned int  FID, uint8_t *buf, int nbyte);


/*desfire command prototypes*/
int authentification(uint8_t key_index,  uint8_t * key1, uint8_t * key2, uint8_t * challenge);

int  isValidPrim     (uint8_t * noPrim, uint8_t * Prim);
void xor             (uint8_t * input1, uint8_t * input2, uint8_t * output);
void buildPrim       (uint8_t * data);
void encrypt         (uint8_t * input, uint8_t * output, uint8_t *key1, uint8_t *key2);


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
    /*libnfc initialization + tag polling*/
    init();
    
    uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
    uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
    size_t abtTx_size;
   
  
    // if (argc < 2){
    //printf("Error : Usage : ./select_app aid\n");
    //return EXIT_FAILURE;
    //}
    
    unsigned int AID = 0x000000;
    printf("Enter the ID of the application you want to select : ");
    scanf("%06x",&AID); // Retrieve the first 6 hexa digits introduced

    if(authentification(0, defaultkey, defaultkey, datatest) == 0)
    {
        printf("Authentication success\n");
    }
    else
    {
        printf("Authentication failed\n");
    }
 


    /*prepare the data*/
    abtTx_size = 9;
    memcpy(abtTx,selectApplication_apdu,abtTx_size);
    abtTx[7] = (AID >> 16) & 0xff; /*data 0*/
    abtTx[6] = (AID >> 8 ) & 0xff; /*data 1*/
    abtTx[5] = AID & 0xff;         /*data 2*/
    
    #ifdef DEBUG
    /*debug message*/
    printf("Select Application 0x%06x\n",AID);
    #endif
    
    /*send the data to the card, the expected status word is 0x91 0x00*/
    if(sendRequest(abtTx, abtTx_size, abtRx, 0x9100, 0) != 0) 
    {
        printf("Failure\n");
        return EXIT_FAILURE;
    }
    printf("Success\n");

    char input[10];

    while (strncmp(input,"exit",5) != 0){
    
      printf("Action to perform on file : [create,delete,list,write,read,exit] ");
      scanf("%s",&input); // Retrieve the action

      if(strncmp(input,"create",6)==0){
	 unsigned int FID = 0x00;
	 printf("Enter the ID of the file you want to create : ");
	 scanf("%02x",&FID); // Retrieve the first 2 hexa digits introduced
     
	 if(createFile(FID) != 0){
	     printf("Failure\n");
	     return EXIT_FAILURE;
	 }
	 printf("Success\n");
      } else if(strncmp(input,"delete",6)==0){
	 unsigned int FID = 0x00;
	 printf("Enter the ID of the file you want to delete : ");
	 scanf("%02x",&FID); // Retrieve the first 2 hexa digits introduced
	 printf("FID = 0x%06x\n",FID);
	 if(deleteFile(FID) != 0)
         {
	   printf("Failure\n");
	   return EXIT_FAILURE;
	 }
	 printf("Success\n");
	 /* LIST  */
      } else if(strncmp(input,"list",4)==0){
	unsigned int list[28];
	int count;
	int i=0;
	if(listFile(list,&count) != 0){
	     printf("Failure\n");
	     return EXIT_FAILURE;
	 }
	printf("Number of file = %d\n",count);
	for (i; i< count ; i++){
	  printf("Fil [FID] == 0x%02x\n",list[i]);
	}
	 printf("Success\n");
      } else if(strncmp(input,"read",4)==0){
	unsigned int  FID;
	uint8_t *buf;
	int nbyte;

	printf("Enter the ID of the file you want to read : ");
	scanf("%02x",&FID); // Retrieve the first 2 hexa digits introduced

	printf("Enter the size of the file you want to read : ");
	scanf("%02x",&nbyte); // Retrieve the first 2 hexa digits introduced

	if(readFile(FID,buf, nbyte) != 0){
	     printf("Failure\n");
	     return EXIT_FAILURE;
	 }
	 printf("Success\n");

      /* WRITE */
      } else if(strncmp(input,"write",4)==0){
	unsigned int  FID;
	uint8_t buf[4];
	buf[0]=0xfa;
	buf[1]=0xce;
	buf[2]=0xb0;
	buf[3]=0x0c;
	int nbyte = 4;

	printf("Enter the ID of the file you want to write : ");
	scanf("%02x",&FID); // Retrieve the first 2 hexa digits introduced

	
	
	//	printf("Enter the size of the file you want to write : ");
	//scanf("%02x",&FID); // Retrieve the first 2 hexa digits introduced

	if(writeFile(FID,buf,nbyte) != 0){
	     printf("Failure\n");
	     return EXIT_FAILURE;
	 }
	 printf("Success\n");
      } //end else if

    } // end while
    return EXIT_SUCCESS;

}

/*TODO put create and select app here */


int createFile(unsigned int FID)
{// /!\ attention size
  uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
  uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
  size_t abtTx_size;
  
  /*check the AID*/
  if(FID > 0xff )/*|| FID < 0x00) the integer is unsigned, its value can not be below 0*/
    {
      fprintf(stderr, "Invalid FID, expected a value between 0x00 and 0xff, got 0x%06x\n",FID);
      return -1;
    }
  
  /*prepare the data*/
  abtTx_size = 13;
  memcpy(abtTx,createFile_apdu,abtTx_size);
  abtTx[5] = FID & 0xff;         /*file descriptor*/
  
  unsigned int filesize = 42;
  abtTx[11] = (filesize >> 16) & 0xff; /*data 0*/
  abtTx[10] = (filesize >> 8 ) & 0xff; /*data 1*/
  abtTx[9] = filesize & 0xff;         /*data 2*/
  
  int read;
  printf("Enter key for Reading/Writeing : ");
  scanf("%02x",&read); // Retrieve the first 2 hexa digits introduced
  abtTx[8] = read & 0xff;

  int randw;
  printf("Enter key for Read&Write/Change : ");
  scanf("%02x",&randw); // Retrieve the first 2 hexa digits introduced
  abtTx[7] = randw & 0xff;
 

  int comm = 0x00;
  printf("Enter communication settings (deffault 0x00) : ");
  scanf("%02x",&comm); // Retrieve the first 2 hexa digits introduced
  abtTx[6] = comm & 0xff;
 


  #ifdef DEBUG
  /*debug message*/
  printf("Create File 0x%02x\n",FID);
  #endif
    
  /*send the data to the card, the expected status word is 0x91 0x00*/
  if(sendRequest(abtTx, abtTx_size, abtRx, 0x9100, 0) != 0) {return -1;}
  
  return 0;
}

int deleteFile(unsigned int FID) /**/
{
    uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
    uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
    size_t abtTx_size;
    
    /*check the AID*/
    if(FID > 0xff )/*|| FID < 0x00) the integer is unsigned, its value can not be below 0*/
    {
        fprintf(stderr, "Invalid FID, expected a value between 0x00 and 0xff, got 0x%06x\n",FID);
        return -1;
    }
    
    /*prepare the data*/
    abtTx_size = 7;
    memcpy(abtTx,deleteFile_apdu,abtTx_size);
    abtTx[5] = FID & 0xff;         /*file descriptor*/
    
    #ifdef DEBUG
    /*debug message*/
    printf("Delete File 0x%02x\n",FID);
    #endif
    
    /*send the data to the card, the expected status word is 0x91 0x00*/
    if(sendRequest(abtTx, abtTx_size, abtRx, 0x9100, 0) != 0) {return -1;}
    
    return 0;
}

int listFile(unsigned int *outputList, int *outputCount)/**/
{
    uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
    uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
    size_t abtTx_size;
    
    
    /*prepare the data*/
    abtTx_size = 5;
    memcpy(abtTx,listFile_apdu,abtTx_size);
    
    #ifdef DEBUG
    /*debug message*/
    printf("List File \n");
    #endif
    
    int status;
    int length;
    /*send the data to the card, the expected status word is 0x91 0x00*/
    if(listRequest(abtTx, abtTx_size, abtRx, &status,&length) != 0) {return -1;}
    //printf("0x9100 -> %d  &&  0x91AF -> %d\n",status == 0x9100, status == 0x91AF);
    if (status != 0x9100){return -1;}
    unsigned int tmp;
    int count = 0;
    for (count; count<length; count++){
      tmp =  abtRx[count];
      outputList[count]=tmp;
    }
    // now count == number of app already given
    *outputCount = count;

    return 0;
}

int readFile(unsigned int  FID, uint8_t *buf, int nbyte)/**/
{
    uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
    uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
    uint8_t abtTx2[MAX_FRAME_LEN]; /*input buffer          */
    size_t abtTx_size;
    
    
    /*prepare the data*/
    abtTx_size = 13;
    memcpy(abtTx,readFile_apdu,abtTx_size);
    
    abtTx[5] = FID & 0xff;         /*file descriptor*/
  
    /* abtTx[8] = (filesize >> 16) & 0xff; //data 0
    abtTx[7] = (filesize >> 8 ) & 0xff; //data 1
    abtTx[6] = filesize & 0xff;         //data 2
    */

    abtTx[11] = (nbyte >> 16) & 0xff; /*data 0*/
    abtTx[10] = (nbyte >> 8 ) & 0xff; /*data 1*/
    abtTx[9] = nbyte & 0xff;         /*data 2*/

    #ifdef DEBUG
    /*debug message*/
    printf("Read File \n");
    #endif
    
    int status;
    int length;
    
    /*send the data to the card, the expected status word is 0x91 0x00*/
    if(listRequest(abtTx, abtTx_size, abtRx, &status,&length) != 0) {return -1;}
    //printf("0x9100 -> %d  &&  0x91AF -> %d\n",status == 0x9100, status == 0x91AF);
    if (status != 0x9100 && status != 0x91AF){return -1;}
    unsigned int tmp;
    int count = 0;
    for (count; count<length; count++){
      printf("%02x",abtRx[count]);
    }


   /*prepare the data*/
    abtTx_size = 5;
    memcpy(abtTx2,readFile2_apdu,abtTx_size);
    
     #ifdef DEBUG
    /*debug message*/
    //printf("Read Another Seg of File \n");
    #endif

    while (status == 0x91AF){
   
      /*send the data to the card, the expected status word is 0x91 0x00*/
      if(listRequest(abtTx, abtTx_size, abtRx, &status,&length) != 0) {return -1;}
      //printf("0x9100 -> %d  &&  0x91AF -> %d\n",status == 0x9100, status == 0x91AF);
      if (status != 0x9100 && status != 0x91AF){return -1;}
      unsigned int tmp;
      int count = 0;
      for (count; count<length; count++){
	printf("%02x",abtRx[count]);
      }

    
    }

    printf("\n");
    printf("ENDOFFILE !\n");
 
    return 0;
}

int writeFile(unsigned int  FID, uint8_t *buf, int nbyte)
{
    uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
    uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
    uint8_t abtTx2[MAX_FRAME_LEN]; /*input buffer          */
    size_t abtTx_size;

    memcpy(abtTx,writeFile_apdu,4);
    

    abtTx[5] = FID & 0xff;         /*file descriptor*/

    int offset = 0;
    abtTx[8] = (offset >> 16) & 0xff; /*data 0*/
    abtTx[7] = (offset >> 8 ) & 0xff; /*data 1*/
    abtTx[6] = offset & 0xff;         /*data 2*/


    abtTx[11] = (nbyte >> 16) & 0xff; /*data 0*/
    abtTx[10] = (nbyte >> 8 ) & 0xff; /*data 1*/
    abtTx[9] = nbyte & 0xff;         /*data 2*/

    int nWritten = 0;
    /*prepare the data*/
    if (nbyte > 52){
      abtTx_size = 13+52;
      memcpy(&abtTx[12],buf,52);
      abtTx[11+52+1] = 0x00;
      abtTx[4] = 59;
      nWritten = 52;
    } else {
      abtTx_size = 13 + nbyte;
      memcpy(&abtTx[12],buf,nbyte);
      abtTx[11+nbyte+1] = 0x00;
      abtTx[4] = 7 + nbyte;
      nWritten = nbyte;
    }

   
    #ifdef DEBUG
    /*debug message*/
    printf("Write File \n");
    #endif
    
    int status;
    int length;
    /*send the data to the card, the expected status word is 0x91 0x00*/
    if(listRequest(abtTx, abtTx_size, abtRx, &status,&length) != 0) {return -1;}
 



    while (status == 0x91AF){

      /*prepare the data*/
      abtTx_size = 4;
      memcpy(abtTx2,writeFile2_apdu,abtTx_size);
    
      #ifdef DEBUG
      /*debug message*/
      printf("Wrote  Another Seg of File \n");
      #endif

      if (nbyte - nWritten > 59 ){
	abtTx[4]=59;
	abtTx[5+59]=0x00;
	abtTx_size = 6+59;
	nWritten = nWritten + 59;
      } else {
	abtTx[4]=nbyte-nWritten;
	abtTx[5+nbyte-nWritten]=0x00;
	abtTx_size = 6+abtTx[4];
	nWritten = nWritten + nbyte;
      }
      memcpy(&abtTx[5],&buf[nWritten-abtTx[4]],abtTx[4]);
      
	
   
      /*send the data to the card, the expected status word is 0x91 0x00*/
      if(listRequest(abtTx, abtTx_size, abtRx, &status,&length) != 0) {return -1;}
    
    }

    if (status != 0x9100){return -1;}
    printf("END of writing !\n");
 
    return 0;
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


int listRequest(uint8_t * abtTx, size_t abtTx_size, uint8_t * abtRx, int *expected_status_word, int *length)
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
    
    *length = res-2;
    /*check the status word*/    
    if(res > 1) 
    {
        /*the status word is build with the two last bytes of the output*/
        *expected_status_word = abtRx[res-2];
        *expected_status_word = ((*expected_status_word)<<8) + abtRx[res-1];
        
        //if(status_word != expected_status_word)
        //{
        //    fprintf(stderr, "Invalid status word on request sending, expected 0x%04x got 0x%04x\n",expected_status_word,status_word);
        //    return -1;
        //}
    }
    else
    {
        fprintf(stderr, "Invalid response length on request sending, expected 10 bytes got %d\n",res);
        return -1;
    }
    
    /*check the data length*/
    //if((res-2) != expected_data_length) /*check the data length*/
    //{
    //    fprintf(stderr, "Invalid length of the response data on request sending, expected %d bytes got %d\n", expected_data_length,res-2);
    //    return -1;
    // }
    
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
    memcpy(abtTx,auth_pass1,abtTx_size);
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
    memcpy(abtTx,auth_pass2,abtTx_size);
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










