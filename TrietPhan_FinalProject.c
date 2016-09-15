/* Triet Phan CS 350, Spring 2016
   Final Project

Extra Credit:
- Initial memory dump start at the Origin and wrap around xFFFF to x0000 to origin-1
- Small values are printed in decimal; large ones in hex
- Print ADD calculations in decimal
- Change the PC
- Change the Running Flag back to 1
*/

#include <stdio.h>
#include <stdlib.h>       // For error exit()

// CPU Declarations -- a CPU is a structure with fields for the
// different parts of the CPU.
typedef short int Word;
typedef unsigned short int Address;

#define MEMLEN 65536
#define NREG 8
#define CC_NEGATIVE 4
#define CC_ZERO 2
#define CC_POSITIVE 1

typedef struct {
  Word mem[MEMLEN];
  Word reg[NREG];
  Address pc;          // Program Counter
  int running;         // running = 1 iff CPU is executing instructions
  Word ir;             // Instruction Register
  Word opcode;          //   opcode field
  Address ORIGIN;
  Word condition_code;
} CPU;

int main(int argc, char *argv[]);
void initialize_CPU(CPU *cpu);
void initialize_memory(int argc, char *argv[], CPU *cpu);
FILE *get_datafile(int argc, char *argv[]);

void dump_CPU(CPU *cpu);
void dump_memory(CPU *cpu);
void dump_registers(CPU *cpu);

Word extract_bits(Word value, int start, int end);
Word print_offset(Word value, int start, int end);
Word get_offset(CPU *cpu, int start, int end);
char get_cc(CPU *cpu);
void update_cc(CPU *cpu);
void update_cc_trap(CPU *cpu);

void print_assembler(Address val);
void print_BR(Address val, Word offset);
void print_ADD(Address val);
void print_AND(Address val);
void print_JSR(Address val);
void print_LDR(Address val);
void print_STR(Address val);

int read_execute_command(CPU *cpu);
int execute_command(char cmd_char, char *cmd_line, CPU *cpu);

void one_instruction_cycle(CPU *cpu);
void many_instruction_cycles(int nbr_cycles, CPU *cpu);
void help_message(void);
void jump_pc(char *cmd_line, CPU *cpu);

void exec_BR(CPU *cpu);
void exec_ADD(CPU *cpu);
void exec_LD(CPU *cpu);
void exec_ST(CPU *cpu);
void exec_JSR(CPU *cpu);
void exec_AND(CPU *cpu);
void exec_LDR(CPU *cpu);
void exec_STR(CPU *cpu);
void exec_RTI(CPU *cpu);
void exec_NOT(CPU *cpu);
void exec_LDI(CPU *cpu);
void exec_STI(CPU *cpu);
void exec_JMP(CPU *cpu);
void exec_LEA(CPU *cpu);
void exec_TRAP(CPU *cpu);
void exec_TRAP_GETC(CPU *cpu);
void exec_TRAP_OUT(CPU *cpu);
void exec_TRAP_PUTS(CPU *cpu);
void exec_TRAP_IN(CPU *cpu);
void exec_TRAP_HALT(CPU *cpu);
void bad_opcode(CPU *cpu);

// Main program: Initialize the cpu, and read the initial memory values
int main(int argc, char *argv[]) {
  printf("Triet Phan CS350 Final Project\n");

  CPU cpu_value, *cpu = &cpu_value;
  initialize_memory(argc, argv, cpu);
  initialize_CPU(cpu);
  printf("Origin = x%04hX\n", cpu -> ORIGIN);
  dump_CPU(cpu);
  dump_memory(cpu);

  char *user_input = "> ";
  printf("Executing, type h or ? for help\n%s", user_input);
  int check = read_execute_command(cpu);
  while (!check) {
    printf("%s", user_input);
    check = read_execute_command(cpu);
  }
  return 0;
}

// Initialize the CPU (pc, ir, instruction sign, running flag,
// and the general-purpose registers).
void initialize_CPU(CPU *cpu) {
  for(int i = 0; i < NREG; i++)
    cpu->reg[i] = 0;

  cpu -> ir = 0;
  cpu -> pc = cpu -> ORIGIN;
  cpu -> running = 1;
  cpu -> opcode = 0;
  cpu -> condition_code = CC_ZERO;
}

// Read and dump initial values for memory
void initialize_memory(int argc, char *argv[], CPU *cpu) {
  FILE *datafile = get_datafile(argc, argv);
  // Buffer to read next line of text into
#define BUFFER_LEN 80
  char buffer[BUFFER_LEN];
  int words_read;
  Word value_read;
  char *read_success;    // NULL if reading in a line fails.
  // Getting the origin
  fgets(buffer, BUFFER_LEN, datafile);
  sscanf(buffer, "%hx ", &value_read);

  // Checking if the file start with the number or char, set the ORIGIN
  // If not, read the next line
  int got_origin = 0;
  while(!got_origin){
    if((buffer[0] >= '0' && buffer[0] <= '9') ||
       (buffer[0] >= 'a' && buffer[0] <= 'z') ||
       (buffer[0] >= 'A' && buffer[0] <= 'Z')){
      cpu -> ORIGIN = (Address) value_read;
      got_origin = 1;
    } else {
      fgets(buffer, BUFFER_LEN, datafile);
      sscanf(buffer, "%hx ", &value_read);
    }
  }

  // Reading the file
  Address loc = cpu -> ORIGIN; // Start from the origin
  read_success = fgets(buffer, BUFFER_LEN, datafile);
  while (read_success != NULL) {
    words_read = sscanf(buffer, "%hx", &value_read);
    if(words_read != 1) {
      read_success = fgets(buffer, BUFFER_LEN, datafile);
      continue;
    }
    else {
      cpu -> mem[loc] = value_read;
      ++loc;
    }
    read_success = fgets(buffer, BUFFER_LEN, datafile);
  }
  while (loc != cpu -> ORIGIN) {
    cpu -> mem[loc] = 0;
    loc++;
  }
}

FILE *get_datafile(int argc, char *argv[]) {
  char *default_datafile_name = "default.hex";
  char *datafile_name;
  if (argc == 2) {
    datafile_name = argv[1];
  }
  else if (argc == 1) {
    datafile_name = default_datafile_name;
  }
  else {
    printf("\nInvalid input: use %s filename\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  FILE *datafile = fopen(datafile_name, "r");
  if (datafile == NULL) {
    printf("Can't open file %s\n", datafile_name);
    exit(EXIT_FAILURE);
  }
  printf("Loading %s\n\n", datafile_name);
  return datafile;
}

// dump cpu
void dump_CPU(CPU *cpu) {
  char cc_char = get_cc(cpu);
  printf("CONTROL UNIT:\n");
  printf("PC = x%04hX\t IR = x%04hX\tCC = %c\t\tRUNNING: %d\n", cpu -> pc, cpu -> ir, cc_char, cpu -> running);
  dump_registers(cpu);
}

// dump memory
void dump_memory(CPU *cpu) {
  Address current_location = cpu -> ORIGIN;
  printf("MEMORY (from x%04hX):\n", cpu -> ORIGIN);
  do {
    if(cpu -> mem[current_location] != 0){
      // Print the location, memory value in hex and decimal
      printf("x%04hX: x%04hX %6d\t", current_location, cpu -> mem[current_location], cpu -> mem[current_location]);
      print_assembler(cpu -> mem[current_location]);
    }
  } while(++current_location != cpu -> ORIGIN); // iterate until back to the ORIGIN - 1
  printf("\n");
}

// dump registers
void dump_registers(CPU *cpu) {
  for (int i = 0; i < NREG; i++) {
    printf("R%d: x%04hX %-6d  ", i, cpu -> reg[i], cpu -> reg[i]);
    if (i % 4 == 3)
      printf("\n");
  }
  printf("\n");
}

// Extract the bits value
Word extract_bits(Word value, int start, int end) {
  Word mask = (1 << (end - start)) - 1;
  return (value >> start) & mask;
}

// Print the correct sign of the offset while dumping memory
Word print_offset(Word value, int start, int end) {
  Word offset;
  Word sign_bit = extract_bits(value, end - 1, end);
  if(sign_bit) {
    // If the offset is negative, take 2s complement of first 5 bits
    offset = - (extract_bits(~(value), start, end) + 1);
  }
  else {
    offset = extract_bits(value, start, end);
  }
  return offset;
}

// Get the correct sign of the offset
Word get_offset(CPU *cpu, int start, int end) {
  Word offset;
  Word sign_bit = extract_bits(cpu -> ir, end - 1, end);
  if(sign_bit) {
    offset = - (extract_bits(~(cpu -> ir), start, end) + 1);
  }
  else {
    offset = extract_bits(cpu -> ir, start, end);
  }
  return offset;
}

// Returns a condition code character
// based on current state of condition code
char get_cc(CPU *cpu) {
  char cc_char;
  switch(cpu -> condition_code) {
  case CC_ZERO: cc_char = 'Z'; break;
  case CC_NEGATIVE: cc_char = 'N'; break;
  case CC_POSITIVE: cc_char = 'P'; break;
  default: cc_char = 'E'; printf("Error CC\n"); //Should never happen
  }
  return cc_char;
}

// Update condition code if there is a destination register
void update_cc(CPU *cpu) {
  Address check_reg = extract_bits(cpu -> ir, 9, 12);
  Word value = cpu -> reg[check_reg];
  if(value == 0) {
    cpu -> condition_code = CC_ZERO;
  }
  else if(value > 0) {
    cpu -> condition_code = CC_POSITIVE;
  }
  else {
    cpu -> condition_code = CC_NEGATIVE;
  }
}

// Update condition code when doing Trap call
void update_cc_trap(CPU *cpu){
  cpu -> reg[7] = cpu -> pc;
  Address check_reg = cpu -> reg[7];
  if(check_reg >= 32768){
    cpu -> condition_code = CC_NEGATIVE;
  }
}

// Print out the assembler mnemonics
void print_assembler(Address val){
  int opcode = extract_bits(val, 12, 16); // 4-bit opcode
  Address dest = extract_bits(val, 9, 12); // 3-bit dest
  Address src1 = extract_bits(val, 6, 9); // 3-bit source 1
  Word offset =  print_offset(val, 0, 9); // 9-bit offsets
  Word trap_vector = extract_bits(val, 0, 8); // 8-bit trap vectors

  switch(opcode) {
  case 0: print_BR(val, offset); break; // use the function to print the opcode BR
  case 1: print_ADD(val); break; // use a function to print the opcode ADD
  case 2: printf("%-4s\t R%d, %d ", "LD", dest, offset); break;
  case 3: printf("%-4s\t R%d, %d", "ST", dest, offset); break;
  case 4: print_JSR(val); break; // use a function to print the opcode JSR
  case 5: print_AND(val); break; // use a function to print the opcode AND
  case 6: print_LDR(val); break; // use a function to print the opcode LDR
  case 7: print_STR(val); break; // use a function to print the opcode STR
  case 8: printf("%-4s\t", "RTI"); break;
  case 9: printf("%-4s\t R%d, R%d", "NOT", dest, src1); break;
  case 10: printf("%-4s\t R%d, %d ", "LDI", dest, offset); break;
  case 11: printf("%-4s\t R%d, %d ", "STI", dest, offset); break;
  case 12: printf("%-4s\t R%d", "JMP", src1); break;
  case 13: printf("%-4s\t", "ERR"); break;
  case 14: printf("%-4s\t R%d, %d ", "LEA", dest, offset); break;
  case 15: printf("%-4s\t x%hX", "TRAP", trap_vector); break;
  default: printf("Bad opcode"); break;
  }
  printf("\n");
}

// Print BR function
void print_BR(Address val, Word offset){
  Word mask = extract_bits(val, 9, 12);
  switch(mask){
  case 1: printf("BRP\t %-5d", offset); break;
  case 2: printf("BRZ\t %-5d", offset); break;
  case 3: printf("BRZP\t %-5d", offset); break;
  case 4: printf("BRN\t %-5d", offset); break;
  case 5: printf("BRNP\t %-5d", offset); break;
  case 6: printf("BRNZ\t %-5d", offset); break;
  case 7: printf("BRNZP\t %-5d", offset); break;
  default: printf("NOP\t %-5d", offset); break;
  }
}

// Print ADD function
void print_ADD(Address val){
  Address dest = extract_bits(val, 9, 12);
  Address src1 = extract_bits(val, 6, 9);
  Address src2 = extract_bits(val, 0, 3);
  Word im_agr = extract_bits(val, 5, 6); // 5th bit is used to check if we have an immediate argument
  if(im_agr){
    Word value = 0;
    Word sign_bit = extract_bits(val, 4, 5);
    if(sign_bit) {
      value = - (extract_bits(~(val), 0, 5) + 1);
    }
    else {
      value = extract_bits(val, 0, 5);
    }
    printf("%-4s\t R%d, R%d, %d", "ADD", dest, src1, value);
  } else {
    printf("%-4s\t R%d, R%d, R%d", "ADD", dest, src1, src2);
  }
}

// Print JSR function
void print_JSR(Address val){
  Word pc_offset = extract_bits(val, 11, 12);
  if(pc_offset){
    Word offset = print_offset(val, 0, 11);
    printf("%-4s\t %-5d","JSR", offset);
  } else {
    Word base_reg = extract_bits(val, 6, 9);
    printf("%-4s\t R%d", "JSRR", base_reg);
  }
}

// Print AND function
void print_AND(Address val){
  Address dest = extract_bits(val, 9, 12);
  Address src1 = extract_bits(val, 6, 9);
  Word im_agr = extract_bits(val, 5, 6); // 5th bit is used to check if we have an immediate argument
  if(im_agr){
    Word value = 0;
    Word sign_bit = extract_bits(val, 4, 5);
    if(sign_bit) {
      value = - (extract_bits(~(val), 0, 5) + 1);
    } else {
      value = extract_bits(val, 0, 5);
    }
    printf("%-4s\t R%d, R%d, %d", "AND", dest, src1, value);
  } else {
    Address src2 = extract_bits(val, 0, 3);
    printf("%-4s\t R%d, R%d, R%d", "AND", dest, src1, src2);
  }
}

// Print LDR function
void print_LDR(Address val){
  Word dest = extract_bits(val, 9, 12);
  Word src1 = extract_bits(val, 6, 9);
  Word offset = print_offset(val, 0, 6);
  printf("%-4s\t R%d, R%d, %d", "LDR", dest, src1, offset);
}

// Print STR function
void print_STR(Address val){
  Word dest = extract_bits(val, 9, 12);
  Word src1 = extract_bits(val, 6, 9);
  Word offset = print_offset(val, 0, 6);
  printf("%-4s\t R%d, R%d, %d", "STR", dest, src1, offset);
}

// Read and executes a command
int read_execute_command(CPU *cpu) {
  // Buffer to read next command line
#define CMD_LINE_LEN 80
  char cmd_line[CMD_LINE_LEN];
  char *read_success;		// NULL if reading in a line fails.
  int nbr_cycles;		// Number of instruction cycles to execute
  char cmd_char;		// Command 'q', 'h', '?', 'd', or '\n'
  int done = 0;		// Check the simulator works or not

  read_success = fgets(cmd_line, CMD_LINE_LEN, stdin);
  if (!read_success) {
    done = 1;   // Hit end of file
  }

  // If the sscanf succeeds, execute that many instruction cycles
  // else use sscanf on the cmd_buffer to read in a character cmd_char
  // ('q', 'h', '?', 'd', or '\n') and call execute_command on cmd_char
  sscanf(cmd_line, "%c", &cmd_char);
  if(sscanf(cmd_line, "%d", &nbr_cycles) && cmd_char != '\n') {
    many_instruction_cycles(nbr_cycles, cpu);
  }
  else {
    done = execute_command(cmd_char, cmd_line, cpu); // Added cmd line
  }
  return done;
}

// Executes one command, execute the correct function depends on the commond
int execute_command(char cmd_char, char *cmd_line, CPU *cpu) {
  // Temp checks the input
  Word temp = 0;
  if(cmd_char == '?' || cmd_char == 'h') {
    help_message();
  }
  else if(cmd_char == 'd') {
    dump_CPU(cpu);
    dump_memory(cpu);
  }
  else if(cmd_char == 'j' && sscanf(cmd_line, "j x%hx", &temp) && *(cmd_line + 1) != '\n') {
    jump_pc(cmd_line, cpu);
  }
  else if(cmd_char == '\n') {
    one_instruction_cycle(cpu);
  }
  else if(cmd_char == 'q') {
    printf("Quit the simulator\n");
    return 1;
  }
  else {
    printf("Incorrect command, use h or ? for Simulator help guide\n");
  }
  return 0;
}

// Prints out help message
void help_message(void) {
  printf("Simulator help guide:\n");
  printf("h or ? for help\n");
  printf("q to Quit\n");
  printf("d to dump the control unit and memory\n");
  printf("An integer > 0 will execute that many instruction cycles\n");
  printf("Return, which executes one instruction cycle\n");
  printf("j xAddress\t\tChange PC to new Address and change Running flag to 1\n");
}

// Change the PC and the running flag to 1
void jump_pc(char *cmd_line, CPU *cpu) {
  Address new_pc;
  if(sscanf(cmd_line, "j x%hx", &new_pc)) {
    cpu -> pc = new_pc;
    cpu -> running = 1;
    printf("PC jumps to x%hX\n", cpu -> pc);
  }
}

// Executes many instruction cycles
void many_instruction_cycles(int nbr_cycles, CPU *cpu) {
  int INSANE_NBR_CYCLES = 101;
  int SANE_NBR_CYCLES = 100;

  if (nbr_cycles <= 0) {
    printf("Number of cycles > 0\n");
    return;
  }
  else if (!(cpu->running)) {
    printf("CPU isn't running\n");
    return;
  }
  else if (nbr_cycles >= INSANE_NBR_CYCLES) {
    printf("%d isn't correct, execute %d cycles\n", nbr_cycles, SANE_NBR_CYCLES);
    nbr_cycles = SANE_NBR_CYCLES;
  }
  else {
    printf("Executing\n");
  }
  while (nbr_cycles > 0 && cpu -> running) {
    one_instruction_cycle(cpu);
    nbr_cycles--;
  }
}

// Runs one instruction cycle
void one_instruction_cycle(CPU *cpu) {
  if (!(cpu -> running)) {
    printf("Halted\n");
    return;
  }
  Address instruction_loc = cpu -> pc;  // Instruction's location before pc increment)
  cpu -> ir = cpu -> mem[cpu -> pc];
  (cpu -> pc)++;
  cpu -> opcode = extract_bits(cpu -> ir, 12, 16);
  printf("x%04hX: x%04hX\t", instruction_loc, cpu -> ir); //Print instruction location
  switch(cpu -> opcode) {
  case 0: exec_BR(cpu); break;
  case 1: exec_ADD(cpu); break;
  case 2: exec_LD(cpu); break;
  case 3: exec_ST(cpu); break;
  case 4: exec_JSR(cpu); break;
  case 5: exec_AND(cpu); break;
  case 6: exec_LDR(cpu); break;
  case 7: exec_STR(cpu); break;
  case 8: exec_RTI(cpu); break;
  case 9: exec_NOT(cpu); break;
  case 10: exec_LDI(cpu); break;
  case 11: exec_STI(cpu); break;
  case 12: exec_JMP(cpu); break;
  case 13: printf("ERR\t\t\t; Reserved opcode; ignored\n"); break;
  case 14: exec_LEA(cpu); break;
  case 15: exec_TRAP(cpu); break;
  default: bad_opcode(cpu);
  }
}

// Opcode functions begin
// BR opcode
void exec_BR(CPU *cpu) {
  Word mask = extract_bits(cpu -> ir, 9, 12);
  Word offset = get_offset(cpu, 0, 9);
  print_BR(cpu -> ir, offset);
  char cc_char = get_cc(cpu);

  if(mask & cpu -> condition_code) {
    printf("\t\t; CC = %c; PC <- x%hX + %d = ", cc_char, cpu -> pc, offset);
    cpu -> pc += offset;
    printf("x%hX\n", cpu -> pc);
  }
  else {
    printf("\t\t; CC = %c; no branch\n", cc_char);
  }
}

// ADD opcode
void exec_ADD(CPU *cpu) {
  print_ADD(cpu -> ir);
  Address dest = extract_bits(cpu -> ir, 9, 12);
  Address src1 = extract_bits(cpu -> ir, 6, 9), src2;
  Word im_agr = extract_bits(cpu -> ir, 5, 6); // 5th bit is used to check if we have an immediate argument
  Word value = 0; // running total of values to add
  printf("\t; R%d <- x%hX + ", dest, cpu -> reg[src1]);
  if(im_agr) {
    Word sign_bit = extract_bits(cpu -> ir, 4, 5);
    if(sign_bit) {
      value = - (extract_bits(~(cpu -> ir), 0, 5) + 1);
    } else {
      value = extract_bits(cpu -> ir, 0, 5);
    }
    printf("%d", value); // Print digit values of the first 5 bits
  }
  else {
    src2 = extract_bits(cpu -> ir, 0, 3);
    printf("x%hX", cpu -> reg[src2]); // Print the hex values
    value += cpu -> reg[src2];
  }
  cpu -> reg[dest] = cpu -> reg[src1] + value;
  update_cc(cpu);
  char cc_char = get_cc(cpu);
  printf(" = x%hX; CC = %c\n", cpu -> reg[dest], cc_char);
}

// LD opcode
void exec_LD(CPU *cpu) {
  Address dest = extract_bits(cpu -> ir, 9, 12);
  Word offset = get_offset(cpu, 0, 9);
  Word mem_address = cpu -> pc + offset;

  cpu -> reg[dest] = cpu -> mem[mem_address];
  update_cc(cpu);
  char cc_char = get_cc(cpu);
  printf("LD\t R%d, %-3d\t; R%d <- M[x%hX] = x%hX; CC = %c\n",
         dest, offset, dest, mem_address, cpu -> reg[dest], cc_char);
}

// ST opcode
void exec_ST(CPU *cpu) {
  Address source = extract_bits(cpu -> ir, 9, 12);
  Word offset = get_offset(cpu, 0, 9);
  Word mem_address = cpu -> pc + offset;

  cpu -> mem[mem_address] = cpu -> reg[source];
  printf("ST\t R%d, %-3d\t; M[x%hX] <- x%hX\n",
         source, offset, mem_address, cpu -> reg[source]);
}

// JSR opcode
void exec_JSR(CPU *cpu) {
  print_JSR(cpu -> ir);
  Word pc_offset = extract_bits(cpu -> ir, 11, 12);
  Word offset, base_reg, target;
  // Trying to figure out its JSR or JSSR
  if(pc_offset) {
    offset = get_offset(cpu, 0, 11);
    cpu -> reg[7] = cpu -> pc;
    cpu -> pc += offset;
    printf("\t\t; PC <- x%hX + %d = x%hX, R7 <- x%hX\n",
           cpu -> reg[7], offset, cpu -> pc, cpu -> reg[7]);
  }
  else {
    base_reg = extract_bits(cpu -> ir, 6, 9);
    target = cpu -> reg[base_reg];
    cpu -> reg[7] = cpu -> pc;
    cpu -> pc = target;
    printf("\t\t; PC <- x%hX, R7 <- x%hX\n",
           cpu -> pc, cpu -> reg[7]);
  }
}

// AND opcode
void exec_AND(CPU *cpu) {
  print_AND(cpu -> ir);
  Address dest = extract_bits(cpu -> ir, 9, 12);
  Address src1 = extract_bits(cpu -> ir, 6, 9), src2;
  Word value, imm5;
  Word im_agr = extract_bits(cpu -> ir, 5, 6); // 5th bit is used to check if we have an immediate argument
  if(im_agr) {
    Word sign_bit = extract_bits(cpu -> ir, 4, 5);
    if(sign_bit){
      imm5 = - (extract_bits(~(cpu -> ir), 0, 5) + 1);
    } else {
      imm5 = extract_bits(cpu -> ir, 0, 5);
    }
    printf("\t; R%d <- x%hX & x%04hX = ", dest, cpu -> reg[src1], imm5);
    value = imm5 & cpu -> reg[src1];
  } else {
    src2 = extract_bits(cpu -> ir, 0, 3);
    printf("\t; x%hX & x%04hX = ", cpu -> reg[src1], cpu -> reg[src2]);
           value = cpu -> reg[src1] & cpu -> reg[src2];
  }
  cpu -> reg[dest] = value;
  update_cc(cpu);
  char cc_char = get_cc(cpu);
  printf("x%hX; CC = %c\n", cpu -> reg[dest], cc_char);
}

// LDR opcode
void exec_LDR(CPU *cpu) {
  print_LDR(cpu -> ir);
  Word dest = extract_bits(cpu -> ir, 9, 12);
  Word base_reg = extract_bits(cpu -> ir, 6, 9);
  Word offset = get_offset(cpu, 0, 6);
  Word mem_address = cpu -> reg[base_reg] + offset;

  cpu -> reg[dest] = cpu -> mem[mem_address];
  update_cc(cpu);
  char cc_char = get_cc(cpu);
  printf("\t; R%d <- M[x%hX + %d] = M[x%hX] = x%hX; CC = %c\n",
         dest, cpu -> reg[base_reg], offset, mem_address,
         cpu -> mem[mem_address], cc_char);
}

// STR opcode
void exec_STR(CPU *cpu) {
  print_STR(cpu -> ir);
  Word source = extract_bits(cpu -> ir, 9, 12);
  Word base_reg = extract_bits(cpu -> ir, 6, 9);
  Word offset = get_offset(cpu, 0, 6);

  Word mem_address = cpu -> reg[base_reg] + offset;
  cpu -> mem[mem_address] = cpu -> reg[source];
  printf("\t; M[x%hX + %d] = M[x%hX] <- x%hX\n",
         cpu -> reg[base_reg], offset, mem_address, cpu -> reg[source]);
}

// RTI Opcode
void exec_RTI(CPU *cpu) {
  printf("RTI\t\t\t; Opcode ignored\n");
}

// NOT opcode
void exec_NOT(CPU *cpu) {
  Word dest = extract_bits(cpu -> ir, 9, 12);
  Word source = extract_bits(cpu -> ir, 6, 9);
  printf("NOT\t R%d, R%d\t\t; R%d <- NOT x%hX = ", dest, source, dest, cpu -> reg[source]);
  cpu -> reg[dest] = ~(cpu -> reg[source]);
  update_cc(cpu);
  char cc_char = get_cc(cpu);
  printf("x%04hX; CC = %c\n", cpu -> reg[dest], cc_char);
}

// LDI opcode
void exec_LDI(CPU *cpu) {
  Word dest = extract_bits(cpu -> ir, 9, 12);
  Word offset = get_offset(cpu, 0, 9);
  Word mem_address = cpu -> pc + offset;
  cpu -> reg[dest] = cpu -> mem[cpu -> mem[mem_address]];
  update_cc(cpu);
  char cc_char = get_cc(cpu);
  printf("LDI\t R%d, %-3d\t; R%d <- M[M[x%hX]] = M[x%hX] = x%hX; CC = %c\n",
         dest, offset, dest, mem_address,  cpu -> mem[cpu -> pc + offset],
         cpu -> reg[dest], cc_char) ;
}

// ST opcode
void exec_STI(CPU *cpu) {
  Word source = extract_bits(cpu -> ir, 9, 12);
  Word offset = get_offset(cpu, 0, 9);
  Word mem_address = cpu -> pc + offset;
  cpu -> mem[cpu -> mem[mem_address]] = cpu -> reg[source];
  printf("STI\t R%d, %-3d\t; M[M[x%hX]] = M[x%hX] <- x%hX\n",
         source, offset, mem_address, cpu -> mem[cpu -> pc + offset], cpu -> reg[source]);
}

// JMP opcode
void exec_JMP(CPU *cpu) {
  Address base_reg = extract_bits(cpu -> ir, 6, 9);
  cpu -> pc = cpu -> reg[base_reg];
  printf("JMP\t R%d\t\t; PC <- x%hX\n", base_reg, cpu -> pc);
}

// LEA opcode
void exec_LEA(CPU *cpu) {
  Address dest = extract_bits(cpu -> ir, 9, 12);
  Word offset = get_offset(cpu, 0, 9);

  cpu -> reg[dest] = cpu -> pc + offset;
  update_cc(cpu);
  char cc_char = get_cc(cpu);
  printf("LEA\t R%d, %-3d\t; R%d <- x%hX; CC = %c\n",
         dest, offset, dest, cpu -> reg[dest], cc_char);
}

// TRAP opcode
void exec_TRAP(CPU *cpu) {
  Word trap_vector = extract_bits(cpu -> ir, 0, 8);
  update_cc_trap(cpu);
  printf("TRAP\t x%hX\t\t; ", trap_vector);
  switch(trap_vector) {
  case 32: exec_TRAP_GETC(cpu); break;
  case 33: exec_TRAP_OUT(cpu); break;
  case 34: exec_TRAP_PUTS(cpu); break;
  case 35: exec_TRAP_IN(cpu); break;
  case 37: exec_TRAP_HALT(cpu); break;
  default: printf("Bad TRAP code, "); exec_TRAP_HALT(cpu);
  }
}

// Bad Opcode
void bad_opcode(CPU *cpu) {
  printf("Bad opcode x%hx\n", cpu -> opcode);
}

// TRAP - GETC opcode
void exec_TRAP_GETC(CPU *cpu) {
  cpu -> reg[0] &= 0; // zeroing out the register
  char buffer; // temporary character
  int get_success;
  char cc_char = get_cc(cpu);
  printf("CC= %c; GETC: ", cc_char);
  do
    get_success = scanf("%c", &buffer);
  while (!get_success);
  cpu -> reg[0] = buffer;
  printf("Read %c = %d\n", cpu -> reg[0], cpu -> reg[0]);
  fseek(stdin, 0, SEEK_END);
}

// TRAP - OUT opcode
void exec_TRAP_OUT(CPU *cpu) {
  char cc_char = get_cc(cpu);
  printf("CC = %c; OUT: %d = %c\n", cc_char, cpu -> reg[0], cpu -> reg[0]);
}

// TRAP - PUTS opcode
void exec_TRAP_PUTS(CPU *cpu) {
  Address string_pointer = (Address) cpu -> reg[0];
  char temp;
  char cc_char = get_cc(cpu);
  printf("CC = %c; PUTS: ", cc_char);
  do {
    temp = cpu -> mem[string_pointer];
    printf("%c", temp);
    ++string_pointer;
  } while(temp != '\0');
  printf("\n");
}

// TRAP - IN opcode
void exec_TRAP_IN(CPU *cpu) {
  cpu -> reg[0] &= 0; // zeroing out the register
  char buffer; // temporary character
  int get_success;
  char cc_char = get_cc(cpu);
  printf("CC = %c; IN: Input a character> ", cc_char);
  do
    get_success = scanf("%c", &buffer);
  while (!get_success);
  cpu -> reg[0] = buffer;
  printf("Read %c = %d\n", cpu -> reg[0], cpu -> reg[0]);
  fseek(stdin, 0, SEEK_END);
}

// TRAP - HALT opcode
void exec_TRAP_HALT(CPU *cpu) {
  cpu -> condition_code = CC_POSITIVE;
  char cc_char = get_cc(cpu);
  printf("CC = %c; Halting\n", cc_char);
  cpu -> running = 0;
}
