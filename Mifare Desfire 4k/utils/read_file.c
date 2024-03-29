#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <nfc/nfc.h>
#include <openssl/des.h>
#include <string.h>

#define MAX_FRAME_LEN 264
#define FILE_SIZE 42
#define DEBUG

/*APDU Declaration*/

uint8_t selectApplication_apdu[9] = {0x90,0x5A,0x00,0x00,0x03,0x00,0x00,0x00,0x00};
uint8_t createFile_apdu[13] = {0x90,0xCD,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t deleteFile_apdu[7] = {0x90,0xDF,0x00,0x00,0x01,0x00,0x00};
uint8_t writeFile_partial_apdu[5] = {0x90,0x3D,0x00,0x00,0x00};

/* Dummy Data */
uint8_t dummy_data[13] = {0x43, 0x6c, 0x69, 0x6e, 0x63, 0x68, 0x65, 0x20, 0x54, 0x65, 0x61, 0x6d, 0x21}; // Clinche Team!

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
int writeFile(unsigned int  FID, uint8_t *buf, int nbyte, uint8_t offset);
int selectApp(unsigned int AID);
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
    /*libnfc initialization + tag polling*/
    init();
    int AID=0, FID, ch;
    bool create = false;
    bool delete = false;
    bool list = false;
    bool write = false;

    if(argc<2){
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    while ((ch = getopt (argc, argv, "c:d:s:w:r:l")) != -1) {
        switch (ch) {
            case 'c':
                FID = atoi(optarg);
                create = true;
                break;
            case 'd' :
                FID = atoi(optarg);
                delete=true;
                break;
            case 'l' :
                list = true;
                break;
            case's' :
                AID = atoi(optarg);
                break;
            case 'w':
                FID = atoi(optarg);
                write = true;

        }
    }
    if(AID == 0){
        printf("Error : Specify a correct AID (option -s)\n");
        return EXIT_FAILURE;
    }
    if(selectApp(AID) != 0)
    {
        printf("Failure\n");
        return EXIT_FAILURE;
    }
    if(create){
        if(createFile(FID) != 0)
        {
            printf("Failure\n");
            return EXIT_FAILURE;
        }
    }else if(delete){
        if(deleteFile(FID) != 0)
        {
            printf("Failure\n");
            return EXIT_FAILURE;
        }
    }else if(write){
        if(writeFile(FID,dummy_data,13,0) !=0){
            printf("Failure\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

void usage(char *progname)
{
fprintf (stderr, "usage: %s [-c:d:s:l]\n", progname);
fprintf (stderr, "\nOptions:\n");
fprintf (stderr, "  -c n: Create file with n as FID\n");
fprintf (stderr, "  -d n: Delete file with n as FID\n");
fprintf (stderr, "  -l: Lists all files in the applications selected\n");
fprintf (stderr, "  -s n: select application with n AID, mandatory option\n");
}

int selectApp(unsigned int AID)
{

    uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
    uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
    size_t abtTx_size;
    /*prepare the data*/
    abtTx_size = 9;
    memcpy(abtTx,selectApplication_apdu,abtTx_size);
    abtTx[7] = (AID >> 16) & 0xff; /*data 0*/
    abtTx[6] = (AID >> 8 ) & 0xff; /*data 1*/
    abtTx[5] = AID & 0xff;         /*data 2*/
    /*send the data to the card, the expected status word is 0x91 0x00*/
    if(sendRequest(abtTx, abtTx_size, abtRx, 0x9100, 0) != 0)
    {
        printf("Fail at selecting app %d\n", AID);
        return EXIT_FAILURE;
    }
    printf("Success\n");
    return EXIT_SUCCESS;
}


int createFile(unsigned int FID)
{
    uint8_t abtRx[MAX_FRAME_LEN]; /*output buffer         */
    uint8_t abtTx[MAX_FRAME_LEN]; /*input buffer          */
    size_t abtTx_size;
    /*prepare the data*/
    abtTx_size = 13;
    memcpy(abtTx,createFile_apdu,abtTx_size);
    abtTx[5]=FID & 0xff;
    abtTx[6]=0x00;
    abtTx[7]=0xEE;
    abtTx[8]=0xEE;
    abtTx[9]= FILE_SIZE & 0xff;
    abtTx[10]=(FILE_SIZE >> 8) & 0xff;
    abtTx[11]=(FILE_SIZE >> 16) & 0xff;
    /*send the data to the card, the expected status word is 0x91 0x00*/
    if(sendRequest(abtTx, abtTx_size, abtRx, 0x9100, 0) != 0)
    {
        printf("Fail at creating file %d\n", FID);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
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

    return 0;
}

int readFile(unsigned int  FID, uint8_t *buf, int nbyte)/**/
{

    return 0;
}

int writeFile(unsigned int  FID, uint8_t *buf, int nbyte, uint8_t offset)
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
    abtTx_size = 5;
    memcpy(abtTx,writeFile_partial_apdu,abtTx_size);
    abtTx_size += 8 + nbyte;    /* 5 before input, 7 input header, nbyte date, 1 Le */
    abtTx[4] = 7 + nbyte;         /* Lc field*/
    abtTx[5] = FID & 0xff;         /*file descriptor*/
    abtTx[6] = offset & 0xff;         /*file descriptor*/
    abtTx[7] = (offset >> 8) & 0xff;         /*file descriptor*/
    abtTx[8] = (offset >> 16) & 0xff;         /*file descriptor*/
    abtTx[9] = nbyte & 0xff;         /*file descriptor*/
    abtTx[10] = (nbyte >> 8) & 0xff;         /*file descriptor*/
    abtTx[11] = (nbyte >> 16) & 0xff;         /*file descriptor*/
    int i;
    for(i=12;i<12+nbyte; i++){
        abtTx[i] = buf[i-12];
    }
    abtTx[12+nbyte]=0x00;

    #ifdef DEBUG
    /*debug message*/
    printf("Writing in File 0x%02x\n",FID);
    #endif

    /*send the data to the card, the expected status word is 0x91 0x00*/
    if(sendRequest(abtTx, abtTx_size, abtRx, 0x9100, 0) != 0) {return -1;}

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
