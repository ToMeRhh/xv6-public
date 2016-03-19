// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void consputc(int);

static int panicked = 0;

#define UP_KEY 226
#define DOWN_KEY 227
#define LEFT_KEY 228
#define RIGHT_KEY 229
#define MAX_HISTORY 16

int leftMoves = 0;

char historyArray[MAX_HISTORY][128];
int historyArrayTop = -1; // last filled history index
int currentHistoryIndex = -1;



static struct {
  struct spinlock lock;
  int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}

void
panic(char *s)
{
  int i;
  uint pcs[10];
  
  cli();
  cons.locking = 0;
  cprintf("cpu%d: panic: ", cpu->id);
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for(;;)
    ;
}

void print(char *s){  
  cli();
  cons.locking = 0;

  cprintf(s);
  cprintf("\n");
}

//PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

int delcur = 1;
static void
cgaputc(int c)
{
  int pos;
  
  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == BACKSPACE){
    if(pos > 0) --pos;
  } else if (c==LEFT_KEY){ // left key
    if(pos > 0) --pos;
    delcur = 0;
  } else
    crt[pos++] = (c&0xff) | 0x0700;  // black on white

  if(pos < 0 || pos > 25*80)
    panic("pos under/overflow");
  
  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }
  
  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  if (delcur){
    crt[pos] = ' ' | 0x0700;
  }
  delcur = 1;
}

void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else if (c==LEFT_KEY){ //left
    uartputc('\b');
  } else
    uartputc(c);
  cgaputc(c);
}

#define INPUT_BUF 128
struct {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x


char*
copyFromIndex(char *s, const char *t,int index, int n)
{
  char *os;
  
  os = s;

  int i=0;
  while((n-- > 0) && (t[index] != 0)){
    *s++ = t[index];
    index++;
    i++;
  }

  while(n-- > 0)
    *s++ = 0;

  return os;
}

void
addToHistory(char *line){

  if (historyArrayTop < (MAX_HISTORY - 1)){
    historyArrayTop++;
  }

  int i = historyArrayTop;
  for (; i > 0; i--){
    safestrcpy(historyArray[i], historyArray[i-1], INPUT_BUF); // shift all cells right
  }

  copyFromIndex(historyArray[0], line, (input.r % INPUT_BUF) ,INPUT_BUF);
  

  i = 0;
  while(historyArray[0][i] != '\n'){
    i++;
  }
  historyArray[0][i] = 0;
}

void 
clearLine(){
  while (input.buf[input.e % INPUT_BUF] != 0) consputc(input.buf[input.e++ % INPUT_BUF]);
  while(input.e != input.w &&
        input.buf[(input.e-1) % INPUT_BUF] != '\n'){
    input.e--;
    consputc(BACKSPACE);
  }

  int i = 0;
  for (; i < INPUT_BUF; ++i)
  {
    input.buf[i] = 0;
  }

  leftMoves=0;
  input.e = 0;
  input.r = 0;
  input.w = 0;
}

void
consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      doprocdump = 1;   // procdump() locks cons.lock indirectly; invoke later
      break;
    case C('U'):  // Kill line.
      clearLine();
      break;

    case C('H'): 
    case '\x7f': 
    case LEFT_KEY:  
      if(input.e != input.w){
        input.e--;
        if (c!=LEFT_KEY) { // Backspace
          consputc(BACKSPACE);

          int i = input.e;
          while (input.buf[i % INPUT_BUF] != 0){
            input.buf[i % INPUT_BUF] = input.buf[i+1 % INPUT_BUF];
            i++;
          }

          int n=0;
          i = input.e;
          while (input.buf[i % INPUT_BUF] != 0){
            consputc(input.buf[i++ % INPUT_BUF]);
            n++;
          }
          while (n--){
            consputc(LEFT_KEY);
          }

        }
        else {
          consputc(c);
          leftMoves++;
        }
      }
      break;
    case RIGHT_KEY:
      if (input.buf[(input.e) % INPUT_BUF] != 0){
        delcur = 0;
        consputc(input.buf[input.e++ % INPUT_BUF]);
        leftMoves--;
      }
      break;
    
    case UP_KEY: 
    if(historyArrayTop != -1 && currentHistoryIndex < historyArrayTop) {

      clearLine();
      currentHistoryIndex++;

      if(history(input.buf, currentHistoryIndex) == 0){ // history copied successfuly to input.buf
        
        while (input.buf[input.e % INPUT_BUF] != 0){
          consputc(input.buf[input.e++ % INPUT_BUF]);
        }
      }
    }
      
    break;
    case DOWN_KEY:
      if(historyArrayTop != -1 && currentHistoryIndex >= 0){
        clearLine();
        currentHistoryIndex--;

        if(history(input.buf, currentHistoryIndex) == 0){ // history copied successfuly to input.buf
          
          while (input.buf[input.e % INPUT_BUF] != 0){
            consputc(input.buf[input.e++ % INPUT_BUF]);
          }
        }
      }
      if(currentHistoryIndex == -1){
        clearLine();
      }
    break;
    default:
    uartputc('0'+input.r);
    uartputc('\n');
    uartputc('0'+input.e);
    uartputc('\n');
    uartputc('0'+input.w);
    uartputc('\n');
    uartputc('*');
    uartputc('\n');

      if(c != 0 && input.e-input.r < INPUT_BUF){

        // If the user pressed enter at the middle of the line:
        if (c=='\n' && leftMoves != 0){
          while (leftMoves != 0){
            input.e++;
            leftMoves--;
          }
        }


        // insert a letter at the middle of a line:
        int printline = 0;
        int i;
        if (leftMoves != 0){
          printline = 1;
          // shift the letter to the right:
          for (i = leftMoves; i>=0; i--){
            input.buf[(input.e+i+1) % INPUT_BUF] = input.buf[(input.e+i) % INPUT_BUF];
          }
        }

        // replace '\r' with '\n'
        c = (c == '\r') ? '\n' : c;

        // insert the char into the buffer and print it:
        input.buf[input.e++ % INPUT_BUF] = c;
        consputc(c);

        // deal with EOL, CTRL+D and end of buffer:
        if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
          addToHistory(input.buf);
          input.w = input.e;
          currentHistoryIndex = -1;
          leftMoves=0;
          wakeup(&input.r);
        }

        // printline indicates if the rest of the line should be printed:
       if (printline){
          int n=0;
          for (i=0; i<leftMoves; i++){
            consputc(input.buf[i+input.e % INPUT_BUF]);
            n++;
          }
          // go back left to the point of insertion
          while (n--){
            consputc(LEFT_KEY);
          }
        }
      }
      break;
    }
  }
  release(&cons.lock);
  if(doprocdump) {
    procdump();  // now call procdump() wo. cons.lock held
  }
}

int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(input.r == input.w){
      if(proc->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void
consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  picenable(IRQ_KBD);
  ioapicenable(IRQ_KBD, 0);
}



int
history(char * buffer, int historyId)
{
  if(historyId < 0 || historyId > 15){
    return -2;
  }

  if(historyId > historyArrayTop){
    return -1;
  }

  safestrcpy(buffer, historyArray[historyId], INPUT_BUF);
  
  return 0;
}

