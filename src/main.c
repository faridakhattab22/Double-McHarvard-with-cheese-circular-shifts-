#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processor.h"
#include "parser.h"
#include "registers.h"
#include "sreg.h"
#include "pipeline_if_id.h"
#include "alu_hazards.h"
#include "flush.h"
#include "output.h"

// Main Logic 

// initialise processor 
// initialise registers (start as bubbles) including Sreg in initialisation
// initialise instructions memory
// initialise Data memory
 
// import assembly file

// send assembly file to parser 


// while theres no instructions left 

    // execute 
    // print EX stage

    // decode
    // print ID stage

    // fetch
    // print IF stage

    // print all registers, SREG
    // instructions memory, data memory