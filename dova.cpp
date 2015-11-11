#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <bitset>
#include <iomanip>
#include <stdint.h>
#include <algorithm>

bool hexOutput = false;
bool disassemble = false;
bool binaryOutput = false;
bool programCounter = false;

int pc;

std::string trim(std::string line);

typedef struct Instruction
{
    typedef enum Type
    {
        R, I, J, Error

    } Type;

    std::string opname;
    int opcode;
    int funct;
    Type type;

    typedef enum RegType
    {
        rs, rt, rd

    } RegType;

    std::vector<RegType> regOrder;

    typedef enum Flag
    {
        None, Jump, Offset

    } Flag;

    Flag flag;

    int commaCount;
    bool hasParens;

} Instruction;

std::vector<Instruction> instructions;

Instruction makeInstruction(std::string line, Instruction::Type type, int opcode);
Instruction makeRType(std::string line, int opcode, int funct);
Instruction makeIType(std::string line, int opcode, Instruction::Flag flag = Instruction::None);
Instruction makeJType(std::string line, int opcode);

void initInstructions();
Instruction getInstruction(std::string opname);
Instruction getInstruction(std::bitset<32> bitInstr);

std::bitset<5> pullRegister(std::bitset<32> bitInstr, int lower, int upper);

bool writeInstruction(Instruction instr, std::vector<int> regs, int num, std::ofstream& output);
void writeInstruction(Instruction instr, std::bitset<32> bitInstr, std::ofstream& output);

std::vector<std::string> nameToReg;

void initRegs();
int getRegNum(std::string reg);
std::string getRegName(int num);

typedef struct Label
{
    std::string name;
    int address;
    bool last;

} Label;

Label makeLabel(std::string name);

std::vector<Label> labels;

Label getLabel(std::string name);

void assembler(std::fstream& input, std::ofstream& output);
void disassembler(std::fstream& input, std::ofstream& output);

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "usage: ./dova <inputfile> <outputfile> <options:-xdbp>\n";
        return 0;

    }

    //get the command line parameters
    std::string inputPath(argv[1]);
    std::string outputPath(argv[2]);

    for(unsigned int i = 3; i < argc; i++)
    {
        std::string option = argv[i];
        if(option.find('x') != std::string::npos)
            hexOutput = true;
        
        if(option.find('d') != std::string::npos)
            disassemble = true;
        
        if(option.find('b') != std::string::npos)
            binaryOutput = true;
        
        if(option.find('p') != std::string::npos)
            programCounter = true;

    }

    //if no output type flag is set
    if(!hexOutput && !binaryOutput)
    {
        binaryOutput = true;

    }

    //open the input file
    std::fstream inputFile;
    inputFile.open(inputPath.c_str());

    if(!inputFile)
    {
        std::cout << "failed to open input file: " << inputPath << "\n";
        return 0;

    }

    //open the output file
    std::ofstream outputFile;
    outputFile.open(outputPath.c_str());

    if(!outputFile)
    {
        std::cout << "failed to open output file: " << outputPath << "\n";
        return 0;

    }

    initInstructions();
    initRegs();

    //TODO: call a function based on a command line switch -a assembler -d disassembler.
    if(disassemble)
    {
        disassembler(inputFile, outputFile);

    }
    else
    {
        assembler(inputFile, outputFile);

    }

    return 0;

}

std::string trim(std::string line)
{
    std::string whitespaces (" \t\f\v\n\r");

    //get positions of first and last non whitespace character
    int posf = line.find_first_not_of(whitespaces);

    //trim off the whitespace
    if(posf != std::string::npos)
        line = line.substr(posf);

    int posl = line.find_last_not_of(whitespaces);

    if(posl != std::string::npos)
        line = line.substr(0, posl+1);

    return line;

}

Instruction makeInstruction(std::string line, Instruction::Type type, int opcode)
{
    //parse the line similarly to the assembler
    line = trim(line);

    //screw being careful here
    std::string opname = line.substr(0, line.find(' '));
    
    std::vector<Instruction::RegType> regOrder;

    while(line.find('$') != std::string::npos)
    {
        int pos = line.find('$');
        std::string reg = line.substr(pos, 3); //get the reg name
        line = line.substr(0, pos) + line.substr(pos+3);

        //we only accept $rs, $rt and $rd here
        if(reg == "$rs")
            regOrder.push_back(Instruction::rs);
        else if(reg == "$rt")
            regOrder.push_back(Instruction::rt);
        else if(reg == "$rd")
            regOrder.push_back(Instruction::rd);

    }

    int commas = std::count(line.begin(), line.end(), ',');
    bool parens = line.find('(') != std::string::npos && line.find(')') != std::string::npos;
    

    //build the instruction data
    Instruction instr;
    instr.opname = opname;
    instr.opcode = opcode;
    instr.funct = 0;
    instr.type = type;
    instr.regOrder = regOrder;
    instr.flag = Instruction::None;
    instr.commaCount = commas;
    instr.hasParens = parens;
    
    return instr;

}

Instruction makeRType(std::string line, int opcode, int funct)
{
    Instruction instr = makeInstruction(line, Instruction::R, opcode);
    instr.funct = funct; //assign the function

    return instr;

}

Instruction makeIType(std::string line, int opcode, Instruction::Flag flag)
{
    Instruction instr = makeInstruction(line, Instruction::I, opcode);
    instr.flag = flag; //assign a special function

    return instr;

}

Instruction makeJType(std::string line, int opcode)
{
    Instruction instr = makeInstruction(line, Instruction::J, opcode);
    instr.flag = Instruction::Jump;

    return instr;

}

void initInstructions()
{
    instructions.clear();

    //rtype instructions
    instructions.push_back(makeRType("add $rd, $rs, $rt", 0x0, 0x20));
    instructions.push_back(makeRType("sub $rd, $rs, $rt", 0x0, 0x22));
    instructions.push_back(makeRType("and $rd, $rs, $rt", 0x0, 0x24));
    instructions.push_back(makeRType("or $rd, $rs, $rt", 0x0, 0x25));
    instructions.push_back(makeRType("nor $rd, $rs, $rt", 0x0, 0x27));
    instructions.push_back(makeRType("slt $rd, $rs, $rt", 0x0, 0x2a));
    instructions.push_back(makeRType("sll $rd, $rt, shamt", 0x0, 0x0));
    instructions.push_back(makeRType("srl $rd, $rt, shamt", 0x0, 0x2));
    instructions.push_back(makeRType("jr $rs", 0x0, 0x8));

    //itype instructions
    instructions.push_back(makeIType("addi $rt, $rs, imm", 0x8));
    instructions.push_back(makeIType("andi $rt, $rs, imm", 0xc));
    instructions.push_back(makeIType("ori $rt, $rs, imm", 0xd));
    instructions.push_back(makeIType("beq $rs, $rt, offset", 0x4, Instruction::Jump));
    instructions.push_back(makeIType("bne $rs, $rt, offset", 0x5, Instruction::Jump));
    instructions.push_back(makeIType("lw $rt, offset($rs)", 0x23, Instruction::Offset));
    instructions.push_back(makeIType("sw $rt, offset($rs)", 0x2b, Instruction::Offset));

    //j type instructions
    instructions.push_back(makeJType("j target", 0x2));
    instructions.push_back(makeJType("jal target", 0x3));

}

Instruction getInstruction(std::string opname)
{
    for(unsigned int i = 0; i < instructions.size(); i++)
    {
        if(opname == instructions[i].opname)
        {
            return instructions[i];

        }

    }

    Instruction instr;
    instr.type = Instruction::Error;
    return instr;

}

Instruction getInstruction(std::bitset<32> bitInstr)
{
    //pull the opcode
    std::bitset<6> opcode(0);
    for(unsigned int i = 26, k = 0; i < 32; i++, k++)
    {
        opcode[k] = bitInstr[i];

    }

    //if this has an opcode of 0 we need to get the funct
    std::bitset<6> funct(0);
    if(opcode == std::bitset<6>(0))
    {
        for(unsigned int i = 0, k = 0; i < 6; i++, k++)
        {
            funct[k] = bitInstr[i];

        }

    }

    //find the corresponding instruction
    for(unsigned int i = 0; i < instructions.size(); i++)
    {
        std::bitset<6> instrOpcode(instructions[i].opcode);
        std::bitset<6> instrFunct(instructions[i].funct);
        if(instrOpcode == opcode && instrFunct == funct)
        {
            return instructions[i];

        }

    }

    Instruction instr;
    instr.type = Instruction::Error;
    return instr;

}

std::bitset<5> pullRegister(std::bitset<32> bitInstr, int lower, int upper)
{
    std::bitset<5> reg;
    for(unsigned int i = lower, k = 0; i < upper; i++, k++)
    {
        reg[k] = bitInstr[i];

    }
    return reg;

}

bool writeInstruction(Instruction instr, std::vector<int> regs, int num, std::ofstream& output)
{
    //check if we have enough reg values for this instruction
    if(instr.regOrder.size() != regs.size())
    {
        std::cout << "not enough reg values. expected: " << instr.regOrder.size() << "\n";
        return false;

    }

    //write the opcode which is always 6 bits
    std::bitset<6> opbits(instr.opcode);

    std::stringstream out;
    out << opbits;

    //rtype instruction
    if(instr.type == Instruction::R)
    {
        std::bitset<5> rs(0);
        std::bitset<5> rt(0);
        std::bitset<5> rd(0);

        //set the register values in order
        for(unsigned int i = 0; i < regs.size(); i++)
        {
            Instruction::RegType t = instr.regOrder[i];
            if(t == Instruction::rs)
                rs = std::bitset<5>(regs[i]);
            else if(t == Instruction::rt)
                rt = std::bitset<5>(regs[i]);
            else if(t == Instruction::rd)
                rd = std::bitset<5>(regs[i]);

        }

        std::bitset<5> shamt(num);

        std::bitset<6> funct(instr.funct);

        out << rs << rt << rd << shamt << funct;

    }
    else if(instr.type == Instruction::I)
    {
        //TODO: calculate jump length offset somewhere in here.
        std::bitset<5> rs(0);
        std::bitset<5> rt(0);

        //set the register values in order
        for(unsigned int i = 0; i < regs.size(); i++)
        {
            Instruction::RegType t = instr.regOrder[i];
            if(t == Instruction::rs)
                rs = std::bitset<5>(regs[i]);
            else if(t == Instruction::rt)
                rt = std::bitset<5>(regs[i]);

        }

        std::bitset<16> imm(num);

        out << rs << rt << imm;

    }
    else if(instr.type == Instruction::J)
    {
        std::bitset<26> target(num);
        out << target;

    }

    std::string line;
    out >> line;

    if(programCounter)
    {
        output << "0x" << std::hex << std::setfill('0') << std::setw(8) << pc << "\t";

    }

    //output in hexadecimal format
    if(hexOutput)
    {
        std::bitset<32> hexVal(line);
        output << "0x" << std::hex << std::setfill('0') << std::setw(8) << hexVal.to_ulong();
        if(binaryOutput)
        {
            output << "\t";

        }

    }

    if(binaryOutput)
    {
        output << line;

    }

    output << "\n";

    return true;

}

void writeInstruction(Instruction instr, std::bitset<32> bitInstr, std::ofstream& output)
{
    output << instr.opname << " ";
    if(instr.type == Instruction::R)
    {
        //read all the register values
        std::bitset<5> rs = pullRegister(bitInstr, 21, 26);
        std::bitset<5> rt = pullRegister(bitInstr, 16, 21);
        std::bitset<5> rd = pullRegister(bitInstr, 11, 16);
        std::bitset<5> shamt = pullRegister(bitInstr, 6, 11);

        for(unsigned int i = 0; i < instr.regOrder.size(); i++)
        {
            Instruction::RegType t = instr.regOrder[i];
            if(t == Instruction::rs)
                output << getRegName(rs.to_ulong());
            else if(t == Instruction::rt)
                output << getRegName(rt.to_ulong());
            else if(t == Instruction::rd)
                output << getRegName(rd.to_ulong());

            if(i != instr.regOrder.size()-1)
            {
                output << ", ";

            }

        }

        if(shamt.to_ulong() > 0)
        {
            output << ", " << shamt.to_ulong();

        }

    }
    else if(instr.type == Instruction::I)
    {
        std::bitset<5> rs = pullRegister(bitInstr, 21, 26);
        std::bitset<5> rt = pullRegister(bitInstr, 16, 21);

        //read 16 bit immediate value
        std::bitset<16> imm(0);
        for(unsigned int i = 0, k = 0; i < 16; i++, k++)
        {
            imm[k] = bitInstr[i];

        }

        for(unsigned int i = 0; i < instr.regOrder.size(); i++)
        {
            Instruction::RegType t = instr.regOrder[i];
            if(t == Instruction::rs)
            {
                //handle things like 8($s0)
                if(instr.flag == Instruction::Offset)
                {
                    int immediate = imm.to_ulong();
                    //hackiest hack of all hacks to read the negative number
                    uint16_t full = -1;
                    uint16_t half = full/2;
                    if(immediate > half)
                    {
                        immediate -= full;
                        immediate -= 1;

                    }

                    output << immediate;
                    output << "(" << getRegName(rs.to_ulong()) << ")";

                }
                else
                {
                    output << getRegName(rs.to_ulong());

                }

            }
            else if(t == Instruction::rt)
            {
                output << getRegName(rt.to_ulong());

            }

            if(i != instr.regOrder.size()-1)
            {
                output << ", ";

            }

        }

        if(instr.flag != Instruction::Offset)
        {
            int immediate = imm.to_ulong();
            if(instr.flag == Instruction::Jump)
            {
                //hackiest hack of all hacks to read the negative number
                uint16_t full = -1;
                uint16_t half = full/2;
                if(immediate > half)
                {
                    immediate -= full;
                    immediate -= 1;

                }
                immediate *= 4;

            }

            output << ", " << immediate;

        }

    }
    else if(instr.type == Instruction::J)
    {
        //read 26 bit target address
        std::bitset<26> target(0);
        for(unsigned int i = 0, k = 0; i < 26; i++, k++)
        {
            target[k] = bitInstr[i];

        }

        int value = target.to_ulong();
        value *= 4;
        
        output << value;

    }

    output << "\n";

}

void initRegs()
{
    nameToReg.clear();

    //all the reg names in order
    std::string names = "$zero $at $v0 $v1 $a0 $a1 $a2 $a3 $t0 $t1 "
                        "$t2 $t3 $t4 $t5 $t6 $t7 $s0 $s1 $s2 $s3 $s4 "
                        "$s5 $s6 $s7 $t8 $t9 $k0 $k1 $gp $sp $fp $ra";

    std::stringstream ss(names);

    //separate the reg names for the vector    
    std::string reg;
    while(std::getline(ss, reg, ' '))
    {
        nameToReg.push_back(reg);

    }

    //print reg map for debug
    /*for(unsigned int i = 0; i < nameToReg.size(); i++)
    {
        std::cout << "reg " << i << " is " << nameToReg[i] << "\n";

    }*/

}

int getRegNum(std::string reg)
{
    //find the index of the register
    for(unsigned int i = 0; i < nameToReg.size(); i++)
    {
        if(reg == nameToReg[i])
        {
            return i;

        }

    }

    return -1;

}

std::string getRegName(int num)
{
    return nameToReg[num];

}

Label makeLabel(std::string name)
{
    Label label;
    label.name = name;
    label.address = -1;
    label.last = false;
    return label;

}

Label getLabel(std::string name)
{
    for(unsigned int i = 0; i < labels.size(); i++)
    {
        if(labels[i].name == name)
        {
            return labels[i];

        }

    }

    return makeLabel("erorr");

}

void assembler(std::fstream& input, std::ofstream& output)
{
    pc = 0x00400000;
    std::stringstream ss; //for reading the file the second time.

    //parse for labels and comments
    std::string line;
    while(std::getline(input, line))
    {
        ss << line << "\n";

        //remove comment
        if(line.find('#') != std::string::npos)
        {
            line = line.substr(0, line.find('#'));

        }

        line = trim(line);

        //parse for labels
        if(line.find(':') != std::string::npos)
        {
            int pos = line.find(':');
            std::string label = line.substr(0, pos);

            std::string alphanumeric("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
            if(label.find_first_not_of(alphanumeric) != std::string::npos)
            {
                std::cout << "label error: \"" << label << "\"\n";
                std::cout << "labels may only contain alphanumeric characters\naborting\n";
                return;

            }

            std::string numbers("0123456789");
            if(label.find_first_of(numbers) == 0)
            {
                std::cout << "label error: \"" << label << "\"\n";
                std::cout << "labels may not start with a number\naborting\n";
                return;

            }

            line = line.substr(pos+1);
            line = trim(line);
            labels.push_back(makeLabel(label));

        }

        //if line is empty after trim/remove comment skip
        if(line.size() == 0)
        {
            continue;

        }

        //set the address of the label to the next line of actual code
        if(labels.size() > 0)
        {
            Label& label = labels[labels.size()-1];
            if(label.address < 0)
            {
                label.address = pc;
                //std::cout << "label \"" << label.name << "\" at address " << label.address << "\n";

            }

        }

        pc += 0x000004;

    }

    //check for putting a label at the end of code like an exit label
    for(unsigned int i = 0; i < labels.size(); i++)
    {
        Label& label = labels[i];
        if(label.address < 0)
        {
            label.address = pc - 0x000004;
            label.last = true;

        }

    }

    //reset program counter
    pc = 0x00400000;
    int lineNum = 1;

    while(std::getline(ss, line))
    {
        std::string fullLine = line;
        //remove comment
        if(line.find('#') != std::string::npos)
        {
            line = line.substr(0, line.find('#'));

        }

        line = trim(line);

        //parse for labels
        if(line.find(':') != std::string::npos)
        {
            int pos = line.find(':');
            line = line.substr(pos+1);
            line = trim(line);

        }

        //if line is empty after trim/remove comment skip
        if(line.size() == 0)
        {
            lineNum++;
            continue;

        }

        std::string opname;

        //parse the opname
        if(line.find(' ') != std::string::npos)
        {
            int pos = line.find(' ');
            opname = line.substr(0, pos);
            line = line.substr(pos+1);

        }

        Instruction instr = getInstruction(opname);
        if(instr.type == Instruction::Error)
        {
            std::cout << opname << " is not a valid operation\naborting\n";
            return;

        }

        int commas = std::count(line.begin(), line.end(), ',');
        if(commas != instr.commaCount)
        {
            std::cout << "syntax error on line " << lineNum << ": " << fullLine << "\n";
            std::cout << "missing \',\'\naborting\n";
            return;

        }

        bool parens = line.find('(') != std::string::npos && line.find(')') != std::string::npos;
        if(parens != instr.hasParens)
        {
            std::cout << "syntax error on line " << lineNum << ": " << fullLine << "\n";
            std::cout << "missing \'(\' or \')\'\naborting\n";
            return;

        }

        //parse out the registers by looking for $
        std::vector<int> regs;
        while(line.find('$') != std::string::npos)
        {
            int pos = line.find('$');
            std::string reg = line.substr(pos, 3);
            int len = reg == "$ze" ? 5 : 3; //special case for $zero which is only > 2 letter register
            reg = line.substr(pos, len); //reparse reg incase it's $zero
            line = line.substr(0, pos) + line.substr(pos+len);

            //get the register value
            int regNum = getRegNum(reg);
            if(regNum < 0)
            {
                std::cout << reg << " is not a valid register name\naborting\n";
                return;

            }
            regs.push_back(regNum);

        }

        //parse for immediate/offset/shamt
        bool immediateSet = false;
        int imm = 0;

        //we include - so we can have negatives
        std::string numbers("-0123456789");
        int posf = line.find_first_of(numbers);
        int posl = line.find_last_of(numbers);

        if(posf != std::string::npos)
        {
            immediateSet = true;
            std::stringstream ss(line.substr(posf, posl+1));
            ss >> imm;

        }

        //calculate jump offset if applicable
        if(instr.flag == Instruction::Jump)
        {
            line = trim(line);            
            std::string labelName = line;
            std::string alphanumeric("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
            int posl = line.find_last_not_of(alphanumeric);
            if(posl != std::string::npos)
            {
                labelName = line.substr(posl+1);

            }

            Label label = getLabel(labelName);
            if(label.address < 0)
            {
                if(!immediateSet)
                {
                    std::cout << "label " << labelName << " does not exist\naborting\n";
                    return;

                }
                else
                {
                    imm /= 4;

                }

            }
            else
            {
                immediateSet = true;

                //calculate the jump offset
                if(instr.type == Instruction::I)
                {
                    int offset = label.address - pc;
                    imm = offset;

                    imm /= 4;
                    if(imm > 0 || imm < 0)
                    {
                        imm -= 1;

                    }

                    if(label.last)
                        imm++;

                }
                else if(instr.type == Instruction::J)
                {
                    imm = label.address;
                    imm /= 4;
                    if(label.last)
                        imm++;

                }


            }

        }

        if((instr.type == Instruction::I || instr.type == Instruction::J) && !immediateSet)
        {
            std::cout << "immediate/offset/label expected none found\naborting\n";
            return;

        }

        //an error occured while writing the instruction it will be printed out
        if(!writeInstruction(instr, regs, imm, output))
        {
            return;

        }

        pc += 0x000004;
        lineNum++;

    }

}

//this will only work for binary for now
void disassembler(std::fstream& input, std::ofstream& output)
{
    std::string fullFile;
    
    //read the full file in

    std::string line;
    while(std::getline(input, line))
    {
        fullFile += line;

    }

    //read the input 32 characters at a time
    while(fullFile.size() >= 32)
    {
        std::string bitstring = fullFile.substr(0, 32);
        fullFile = fullFile.substr(32);

        std::bitset<32> bitInstr(bitstring);
        Instruction instr = getInstruction(bitInstr);

        if(instr.type == Instruction::Error)
        {
            std::cout << "instruction not supported by this disassembler.\n";
            return;

        }

        writeInstruction(instr, bitInstr, output);

    }

}
