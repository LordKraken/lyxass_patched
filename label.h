#define MAX_LABEL_LEN  32
#define MAX_LOCAL_LABELS  512
#define MAX_MACRO_LABELS  512
#define MAX_GLOBAL_LABELS 8192
#define MAX_MACROS        1024

/* label-types */
#define NORMAL	       0x01   // Label w/ or w/o colon
#define LOCAL	       0x02   // label starting with dot
#define MACRO	       0x04   // label starting with dot and backslash
#define GLOBAL	       0x08   // label starting with double colon
#define VARIABLE       0x10   // normal label, can be redefined
#define UNSURE         0x20   // normal label w/o colon, could be a opcode
#define REGISTER       0x40   // register label ie. usable instead of a register
#define UNSOLVED       0x80   // label used befor definition

typedef struct label_s{
  int len;
  int type;
  long value;
  char *srcptr;
  int line;
  int file;
  int count;
  struct label_s *next;
  struct label_s *last;
  char name[MAX_LABEL_LEN+2];
} LABEL;


LABEL * DefineLabel(LABEL *, int *);
LABEL * FindLabel(LABEL *, long *);
int FindMacro(char *, LABEL **);

void DumpGlobals();
void DumpLocals();
void InitLabels();
void writeSymbols(char *);
