/***************************************************************************
 * mamou.h: main header file
 *
 * $Id$
 *
 * The Mamou Assembler - A Hitachi 6309 assembler
 *
 * (C) 2004 Boisy G. Pitre
 ***************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

#include "bp_types.h"
#include "bp_errno.h"
#include "bp_kal.h"
#include "bp_dal.h"
#include "cocopath.h"

#define ERR     (-1)


#define FNAMESIZE	512
#define MAXBUF  1024
#define MAXOP   10      /* longest mnemonic */
#define MAXLAB  32
#define E_LIMIT 16
#define P_LIMIT 64

/*      Character Constants     */
#define TAB     '\t'
#define BLANK   ' '
#define EOS     '\0'


/*      Opcode Classes          */
typedef enum _opcode_class
{
     INH,			/* Inherent                     */
     GEN,			/* General Addressing           */
     IMM,			/* Immediate only               */
     REL,			/* Short Relative               */
     P2REL,			/* Long Relative                */
     P1REL,			/* Long Relative (LBRA and LBSR)*/
     NOIMM,			/* General except for Immediate */
     P2GEN,			/* Page 2 General               */
     P3GEN,			/* Page 3 General               */
     RTOR,			/* Register To Register         */
     INDEXED,		/* Indexed only                 */
     RLIST,			/* Register List                */
     P2NOIMM,		/* Page 2 No Immediate          */
     P2INH,			/* Page 2 Inherent              */
     P3INH,			/* Page 3 Inherent              */
     GRP2,			/* Group 2 (Read/Modify/Write)  */
     LONGIMM,		/* Immediate mode takes 2 bytes */
     BTB,			/* Bit test and branch          */
     SETCLR,			/* Bit set or clear             */
     CPD,			/* compare d               6811 */
     XLIMM,			/* LONGIMM for X           6811 */
     XNOIMM,			/* NOIMM for X             6811 */
     YLIMM,			/* LONGIMM for Y           6811 */
     YNOIMM,			/* NOIMM for Y             6811 */
     FAKE,			/* convenience mnemonics   6804 */
     APOST,			/* A accum after opcode    6804 */
     BPM,			/* branch reg plus/minus   6804 */
     CLRX,			/* mvi x,0                 6804 */
     CLRY,			/* mvi y,0                 6804 */
     LDX,			/* mvi x,expr              6804 */
     LDY,			/* mvi y,expr              6804 */
     MVI,			/* mvi                     6804 */
     EXT,			/* extended                6804 */
     BIT,			/* bit manipulation        6301 */
     SYS,			/* syscalls (really swi)        */
     PSEUDO,			/* Pseudo ops                   */
     P2RTOR,			/* Page 2 register to register  */
     P3RTOR,			/* Page 3 register to register  */
     P3IMM,			/* Page 3 immediate only	*/
     P3NOIMM			/* Page 3 No Immediate          */
} opcode_class;


/*
 *      pseudo --- pseudo op processing
 */
typedef enum _pseudo_class
{
     RMB,             /* Reserve Memory Bytes         */
     FCB,             /* Form Constant Bytes          */
     FDB,             /* Form Double Bytes (words)    */
     FCC,             /* Form Constant Characters     */
     ORG,             /* Origin                       */
     EQU,             /* Equate                       */
     ZMB,             /* Zero memory bytes            */
     FILL,            /* block fill constant bytes    */
     OPT,             /* assembler option             */
     NULL_OP,         /* null pseudo op               */
     PAGE,            /* new page                     */
     FCS,             /* Form Constant with Hi-Bit    */
     IFP1,            /* If pass1 conditional         */
     IFP2,            /* If pass2 conditional         */
     IFEQ,            /* If zero conditional          */
     IFNE,            /* if !zero conditional         */
     IFGE,            /* If zero conditional          */
     IFGT,            /* if !zero conditional         */
     IFLE,            /* If zero conditional          */
     IFLT,            /* if !zero conditional         */
     ENDC,            /* End condtional               */
     MOD,             /* Start OS-9 module            */
     EMOD,            /* End OS-9 module (CRC)        */
     USE,             /* include another source file  */
     ELSE             /* else from if                 */
} pseudo_class;


struct filestack
{
	coco_path_id	fd;
	char			file[FNAMESIZE];
	BP_int32		current_line;
	BP_int32		num_blank_lines;
	BP_int32		num_comment_lines;
	BP_Bool			end_encountered;
};

struct link
{	/* linked list to hold line numbers */
	BP_int32		L_num; /* line number */
	struct link		*next; /* pointer to next node */
};

struct nlist
{	/* basic symbol table entry */
	BP_char			*name;
	BP_int32		def;
	BP_Bool			overridable;
	struct nlist	*Lnext ; /* left node of the tree leaf */
	struct nlist	*Rnext; /* right node of the tree leaf */ 
	struct link		*L_list; /* pointer to linked list of line numbers */
};

struct orglist
{
	BP_int32		org;
	BP_int32		size;
};

struct oper
{	/* an entry in the mnemonic table */
	char    *mnemonic;      /* its name */
	char    class;          /* its class */
	int     opcode;         /* its base opcode */
	char    cycles;         /* its base # of cycles */
	char    h6309;          /* its processor class (0 = 6809, 1 = 6309) */
	int	(*func)();	/* function */
};


/* Assembler state */
typedef struct _assembler
{
	int					num_errors;					/* total number of errors */
	int					num_warnings;				/* assembler warnings */
	int					cumulative_blank_lines;		/* blank line count across all files */
	int					cumulative_comment_lines;   /* comment line count across all files */
	int					cumulative_total_lines;		/* total line count across all files */
	char				input_line[MAXBUF];			/* input line buffer */
	char				label[MAXLAB];				/* label on current line */
	char				Op[MAXOP];					/* opcode mnemonic on current line */
	char				operand[MAXBUF];			/* remainder of line after op */
	char				comment[MAXBUF];			/* comment after operand, or entire line */
	char				*optr;						/* pointer into current operand field */
	int					force_word;					/* Result should be a word when set */
	int					force_byte;					/* result should be a byte when set */
	int					program_counter;			/* Program Counter */
	int					data_counter;				/* data counter */
	int					old_program_counter;		/* Program Counter at beginning */
	int					DP;							/* Direct Page pointer */
	int					allow_warnings;				/* allow assembler warnings */
	int					last_symbol;				/* result of last symbol_find */
	int					pass;						/* current pass */
	struct filestack	*current_file;
	BP_int32			use_depth;					/* depth of includes/uses */
#define MAXAFILE	16
	int					file_index;
	char				*file_name[MAXAFILE];		/* assembly file name on cmd line */
	int					current_filename_index;		/* file number count            */
#define INCSIZE 16
	int					include_index;
	char				*includes[INCSIZE];	
	int					Ffn;						/* forward ref file #           */
	int					F_ref;						/* next line with forward ref   */
	char				**arguments;				/* pointer to file names        */
	int					E_total;					/* total # bytes for one line   */
	BP_char				E_bytes[E_LIMIT + MAXBUF];  /* Emitted held bytes           */
	int					E_pc;						/* Pc at beginning of collection*/
	int					P_force;					/* force listing line to include Old_pc */
	int					P_total;					/* current number of bytes collected    */
	BP_char				P_bytes[P_LIMIT + 60];		/* Bytes collected for listing  */
	int					cumulative_cycles;			/* # of cycles per instruction  */
	long				Ctotal;						/* # of cycles seen so far */
	int					N_page;						/* new page flag */
	int					page_number;				/* page number */
	int					o_show_cross_reference;					/* cross reference table flag */
	int					Cflag;						/* cycle count flag */
	int					Opt_C;						/* */
	BP_int32			o_page_depth;				/* page depth */
	BP_Bool				o_show_error;				/* show error */
	int					Opt_F;						/* */
	int					Opt_G;						/* */
	BP_Bool				o_show_listing;				/* listing flag 0=nolist, 1=list*/
	BP_Bool				o_decb;						/* */
	int					Opt_N;						/* */
	BP_Bool				o_quiet_mode;				/* quiet mode */
	int					o_show_symbol_table;		/* symbol table flag, 0=no symbol */
	int					o_pagewidth;						/* */
	int					current_line;				/* line counter for printing */
	int					current_page;				/* page counter for printing */
	int					header_depth;				/* page header number of lines */
	int					footer_depth;				/* page footer of lines */
	int					o_format_only;              /* format only flag, 0=no symbol */
	int					o_debug;					/* debug flag */
	int					o_binaryfile;						/* binary image file output flag */
	int					Hexfil;						/* Intel Hex file output flag */
	unsigned char		Memmap[65536];				/* Memory image of output data */
	FILE				*fd_object;					/* object file's file descriptor*/
	char				object_name[FNAMESIZE];
	u_int				accum;
	char				_crc[3];
	int					do_module_crc;
	int					SuppressFlag;
	int					tabbed;
#define	CONDSTACKLEN	256
	int					conditional_stack_index;
	char				conditional_stack[CONDSTACKLEN];
	int					Preprocess;
	BP_Bool				o_h6309;
#define NAMLEN 64
#define TTLLEN NAMLEN
	char				Nam[NAMLEN];
	char				Ttl[TTLLEN];
	struct nlist		*bucket;            /* root node of the tree */
	struct orglist		orgs[256];
	BP_uint32			current_org;
} assembler;


/* function prototypes */
/* mamou.c */
int main(int argc, char **argv);
void mamou_pass(assembler *as);
int mamou_parse_line(assembler *as);
void process(assembler *as);
void init_globals(assembler *as);

/* h6309.c */
void local_init(void);

/* env.c */
void env_init(assembler *as);

/* eval.c */
BP_Bool evaluate(assembler *as, BP_int32 *result, BP_char **eptr, BP_Bool);

/* ffwd.c */
void fwd_init(assembler *as);
void fwd_deinit(assembler *as);
void fwd_mark(assembler *as);
void fwd_next(assembler *as);
void fwd_reinit(assembler *as);


/* print.c */
void print_line(assembler *as, int override, char infochar, int counter);
void report_summary(assembler *as);
void print_header(assembler *as);
void print_footer(assembler *as);


/* symbol_bucket.c */
int symbol_add(assembler *as, char *str, int val, int override);
struct nlist *symbol_find(assembler *as, char *name, int);
struct oper *mne_look(assembler *as, char *str);
void symbol_dump_bucket(struct nlist *ptr);
void symbol_cross_reference(struct nlist *ptr);


/* util.c */
char *extractfilename(char *pathlist);
BP_Bool alpha(BP_char c);
BP_Bool alphan(BP_char c);
BP_Bool any(BP_char c, BP_char *str);
BP_Bool delim(BP_char c);
BP_Bool eol(BP_char c);
void decb_header_emit(assembler *as, BP_uint32 start, BP_uint32 size);
void decb_trailer_emit(assembler *as, BP_uint32 exec);
void emit(assembler *as, int byte);
void error(assembler *as, char *str);
void eword(assembler *as, int wd);
void f_record(assembler *as);
void fatal(char *str);
void finish_outfile(assembler *as);
int head(char *str1, char *str2);
int hibyte(int i);
int lobyte(int i);
char mapdn(char c);
char *skip_white(char *ptr);
void warn(assembler *as, char *str);

/* pseudo.c */
int	_else(assembler *as),
	_emod(assembler *as),
	__end(assembler *as),
	_endc(assembler *as),
	_equ(assembler *as),
	_fdb(assembler *as),
	_fcb(assembler *as),
	_fcc(assembler *as),
	_fcs(assembler *as),
	_fill(assembler *as),
	_ifeq(assembler *as),
	_ifge(assembler *as),
	_ifgt(assembler *as),
	_ifle(assembler *as),
	_iflt(assembler *as),
	_ifne(assembler *as),
	_ifp1(assembler *as),
	_ifp2(assembler *as),
	_mod(assembler *as),
	_nam(assembler *as),
	_null_op(assembler *as),
	_opt(assembler *as),
	_org(assembler *as),
	_page(assembler *as),
	_rmb(assembler *as),
	_set(assembler *as),
	_setdp(assembler *as),
	_spc(assembler *as),
	_ttl(assembler *as),
	_use(assembler *as),
	_zmb(assembler *as);

/* do9.c */
int	_gen(assembler *as, int opcode),
	_grp2(assembler *as, int opcode),
	_grp2_16(assembler *as, int opcode),
	_indexed(assembler *as, int opcode),
	_inh(assembler *as, int opcode),
	_imm(assembler *as, int opcode),
	_imgen(assembler *as, int opcode),
	_longimm(assembler *as, int opcode),
	_noimm(assembler *as, int opcode),
	_noimm_16(assembler *as, int opcode),
	_p1rel(assembler *as, int opcode),
	_p2gen(assembler *as, int opcode),
	_ldqgen(assembler *as, int opcode),
	_p2inh(assembler *as, int opcode),
	_p2noimm(assembler *as, int opcode),
	_p3noimm(assembler *as, int opcode),
	_p2rel(assembler *as, int opcode),
	_p3gen(assembler *as, int opcode),
	_p3gen8(assembler *as, int opcode),
	_p3inh(assembler *as, int opcode),
	_p3imm(assembler *as, int opcode),
	_rel(assembler *as, int opcode),
	_rlist(assembler *as, int opcode),
	_rtor(assembler *as, int opcode),
	_p2rtor(assembler *as, int opcode),
	_p3rtor(assembler *as, int opcode),
	_sys(assembler *as, int opcode);

