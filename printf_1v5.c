/*
 ************************************************************************************************
 * License Information  	:		GNU   
 *************************************************************************************************
 *
 * File Information
 *    File name				:  		UARTprintf.c
 *    Abstract             	:		This file contains the partial implementation of printf function
 *									Current version supports 'integer' and 'floating' values
 *
 * Compilation Information
 *    Compiler/Assembler   	:   	Currently used DEV_C++. You can use any compiler. Just undef _DEV_C_
 *    Version              	:   	3.2 
 *    
 *
 * Hardware Platform       	:   	Compatible only for 32bit processors
 *
 *  Author(s)           	:   	Aravind D Chakravarti 
 *									Email: aravind.chakravarti@gmail.com
 *                          		
 * <Version Number>  <Author>  	<date>		<defect Number>			<Modification(s)>
 *  1.0             Aravind    	10-02-2016			--			Base Version
 *	1.1				Aravind		20-02-2016			--			Added comments 	
 *  1.2				Aravind		22-02-2016			--			For UART support, receives UART buffer
 *	1.3				Aravind		26-02-2016			--			Some more improvements and testing
 *	1.4				Aravind		17-03-2016			--			To support HEX values
 *	1.5				Aravind		24-03-2016			--			Started supporting ZERO/space padding
 *																some bug fixes.
 *	                         
 * Functions				:		void 	UARTPrintf 			(UINT16	, const INT8 *, ...);
 *									static 	INT8* 	print_int	(INT8*	, INT32*, INT8, INT8);
 *									static 	INT8*	print_float	(INT8*	, INT32*, INT8, INT8);
 *
 * Known issues/limitations	:	1. Current implementation supports float values only upto 2^30,
 *									1073741824 <--> 0 <--> -1073741824
 *								2. When you say %6d for a negative number, then print should be 
 *									<number_of_white_spaces><negative_sign><digits>. But, with some
 *									software limitations it is coming as
 *									<negative_sign><number_of_white_spaces><digits>
 *									Ex. int test = -32767, UARTPrinf("%8d", test) => '-  32767'
 *										int test = -32767, prinf("%8d", test) 	  => '  -32767'
 *								3. zero padding is supported only one digit. i.e., %09.9f not %010.9f
 * 
 ***************************************************************************************************	
 */
 
#define _DEV_C_		/* When compiling with processor specific IDE, 'UNDEF' this */

#ifdef _DEV_C_
#include "stdio.h"	/* standard IO library. Remove this while compiling with processor specific IDE */
#endif

/* Common types */
typedef 	int				INT32;
typedef		unsigned int	UINT32;
typedef		char			INT8;
typedef		unsigned char	UINT8;
typedef 	short			INT16;
typedef		unsigned short	UINT16;
typedef		float			f_32;
typedef		double			f_64;

/* Total number of 'bytes' using UARTPrint function, increase this
to increase 'length' of UARTPrinf function */
#define		MAX_UART_ARRAY_SIZE		200

/* If at all processor is not 32 bit */
#define		MAX_INT_SIZE			(8*sizeof(int))

/* zero */
#define 	ZERO_CHAR				'0'
#define		ZERO					 0
#define		SPACE					' '


/* Function prototypes */
#ifdef _DEV_C_
UINT32 	UARTPrintf 			(UINT16	, INT8 *, ...);	/* Printf function */
#else 
UINT32 	UARTPrintf 			(void (*func) (INT8), const INT8 *, ...);	/* Printf function */
#endif
static 	INT8* 	print_int	(INT8*	, INT32*, INT8, INT8);		/* Extracts integers */ 
static 	INT8*	print_float	(INT8*	, INT32*, INT8, INT8);		/* Extracts floats */

/* Global variables used */
UINT8	use_zero_padding = 0;

#ifdef _DEV_C_
void main ()
{
	
	/* Test Variable */
	f_32 	test_var_float1 = 1073741824.111;		/* 2^22 = 4194304 */
	f_32 	test_var_float2 = -1073741824.111;
	UINT32 	test_var_int1 	= 32767;
	UINT32 	test_var_int2 	= -32767;
	
	/* Place below line in appopriate place at code*/
	UARTPrintf(1, "Hello World\nOur Variable are\nFloat-I  = %f\nFloat-II = %f\nINT1     = %d\nINT2     = %d\nHEX-I    = 0x%X\nHEX-II   = 0x%X\n",\
				test_var_float1, test_var_float2, test_var_int1, test_var_int2, test_var_int1, test_var_int2);
	
	/*Test printf (compiler printf) to see how printf behaves */
	printf ("\n****************************************\n");
	printf ("Hello World\nOur Variable are\nFloat-I  = %f\nFloat-II = %f\nINT1     = %d\nINT2     = %d\nHEX-I    = 0x%X\nHEX-II   = 0x%X\n",\
				test_var_float1, test_var_float2, test_var_int1, test_var_int2, test_var_int1, test_var_int2);
}
#endif

/*
* Function Information
*
* Function Name				:		UARTPrintf 
*
* Function Description		:		This function accepts any number of paramters of any type. Decodes
*									given data if the passing style matches with standard 'printf' 
*									However 'MAX_UART_ARRAY_SIZE' decides maximum number of bytes 
*									or charecters to be printed. Do not change this unless required.
*
* Parameters passed			:		UINT16 UART_index :	Index of the UART to which data has to be 
*									passed
*									const INT8 *format : Pointer to the string which as been passed by
*									called function. Basically it is pointing to the 'stack' where
*									string is residing
*
* Parameters returned		:		UINT32 number_of_bytes : Number of bytes written to UART
*
* Functions called			:		print_int
*									print_float
*
* Global variables used		:		None
*
* Global variables modified	:		None
*
*/
#ifdef _DEV_C_
UINT32 UARTPrintf (UINT16 UART_index, INT8 *format, ...)
#else
UINT32 UARTPrintf (void (*fun) (INT8), const INT8 *format, ...)
#endif
{
	INT8	 	*str_ptr;
	INT32		*data_ptr;
	INT8		UART_data_array[MAX_UART_ARRAY_SIZE] = {ZERO};
	INT8		*UART_data_array_ptr;
	INT8		precision		= 4;
	INT8		zero_pad		= 0;
		
	UINT32		number_of_bytes = ZERO;
	UINT32		current_byte	= ZERO;
	
	/* data_ptr is holding address of variable 'format', so when we 
	do 'format++' we shall point to next passed parameter */
	data_ptr	= (INT32 *)(&format);
	
	/* str_ptr is pointing to data pointed by 'format'
	that is string passed by called function */
	str_ptr 	= (INT8 *)(*data_ptr);
	
	/* Pointing array holds data for UART */
	UART_data_array_ptr = &UART_data_array[ZERO];
	
	
	/* Now 'str_ptr' holds the start of string. I shall point to 
	next paramter */
	data_ptr++;
	
		
	/* Scan till end of string. Check what type of data being passed */
	for ( ; *str_ptr != '\0'; ++str_ptr)
	{
		/* Check for some integer or float parameter */	
		if (*str_ptr == '%')
		{
			/* Now '%' being found. Point to next value which shall give
			some hint about TYPE */
			str_ptr++;
			
			/* By default our precision for float is ; and zero padding is */
			precision 	= 6;
			zero_pad	= 0;
			use_zero_padding = 0;
			
			/* Nothing is there afterwards. Do not proceed ahead */
			if ('\0' == *str_ptr)
			{
				break;
			}
			
			/* Send zero padding and precision information */
			if (((*str_ptr>= '0') && (*str_ptr <= '9')) || ('.' == *str_ptr))
			{
				if (ZERO_CHAR == *str_ptr)
				{
					/* yes do zero padding */
					use_zero_padding = 1;
					str_ptr++;
				}
				if ((*str_ptr>= '0') && (*str_ptr <= '9'))
				{
					zero_pad  =  *str_ptr++ - ZERO_CHAR;
				}
				if (*str_ptr == '.')
				{	
					/* Skip '.'  copy precision*/
					str_ptr++;
					if ((*str_ptr>= '0') && (*str_ptr <= '9'))
					{
						precision =  *str_ptr++ - ZERO_CHAR;
					}
				}
			}
			
			/* Integer, unsigned or hex parameter */
			if ((*str_ptr == 'd') || (*str_ptr == 'u') || (*str_ptr == 'X') || (*str_ptr == 'x'))
			{	
				/* Send the buffer pointer get the current pointer position */	
				UART_data_array_ptr = print_int (UART_data_array_ptr, data_ptr, *str_ptr, zero_pad);
				
				/* Point to next parameter by incrementing by one INT */
				data_ptr++;
			}
			
			/* Float parameter */
			else if (*str_ptr == 'f')
			{		
				UART_data_array_ptr = print_float (UART_data_array_ptr, data_ptr, zero_pad, precision);
				
				/* Float will be by default promoted to double in variadic function 
				That is why even though pointing to type flaot, skip two 'floats'*/
				data_ptr++;
				data_ptr++;
			}
			
			else
			{
				/* If nothing is present then it was just a '%' */
				*UART_data_array_ptr++ = (INT8)*str_ptr;
			}
		}
		
		else
		{
			/* Just a string. Keep copying it to buffer */
			*UART_data_array_ptr++ = (INT8)*str_ptr;
		}
	}
	
	/* How many number of bytes has been passed by UARTPrintf function? */
	number_of_bytes = UART_data_array_ptr - (&UART_data_array[ZERO]);
	
	
	/* Lets print this. 
	Next we can fill array here which will be sending data through 
	particular UART */
	while (current_byte < number_of_bytes)
	{
		//current_byte++;
		#ifdef _DEV_C_
		printf ("%c", UART_data_array[current_byte++]);
		#else
		fun (UART_data_array[current_byte++]);
		#endif
	}
	
	return (number_of_bytes);
}

/*
* Function Information
*
* Function Name				:		print_int 
* Function Description		:		This function accepts the integer and converts them 
*									into digits, fills there respective ascii values to passed
*									array.
*
* Parameters passed			:		INT8 *UART_data_array_ptr : Points to array which contains all 
*									users data
*									INT32 *data_ptr	: Points to the integer
*									INT8  sign_bit	: if it is 'd' then it will print both signed & 
*									unsigned if it is 'u' then it will print only unsigned
*									INT8 zero_pad  : How much zero padding we have to do? This is entered
*									by user. If zero_pad length is less than number of digits in the
*									number there will not be any effect
*
* Parameters returned		:		INT8* UART_data_array_ptr : Pointer, which is pointing to last 
*									updated array location
*
*  Functions called			:		None
*
* Global variables used		:		None
*
* Global variables modified	:		None
*
*/
static INT8* print_int	(INT8 *UART_data_array_ptr, INT32 *data_ptr, INT8 sign_bit, INT8 zero_pad)
{
	UINT32	integer_data	= ZERO;
	UINT32	integer_data_copy	= ZERO;
	/* This is going to hold the 'digits' temporarily*/
	INT32	int_char[MAX_INT_SIZE];
	UINT8	int_char_index 	= ZERO;
	UINT16	divide_factor	= ZERO;
	
	divide_factor = (((sign_bit == 'X')||(sign_bit == 'x')) ? 16 : 10) ;
	
	/* Check if we have to print negative number */		
	if ((sign_bit == 'd') && (*data_ptr < ZERO))
	{
		integer_data	=	-1*(*data_ptr++);
		*UART_data_array_ptr++ = '-';
		zero_pad--;
	}
	
	/* We are going to print only positive numbers */	
	else if (sign_bit == 'u') 
	{
		if (*data_ptr >= ZERO)
		{
			integer_data	=	(INT32)*data_ptr++;
		}
		else
		{
			integer_data	=	(UINT32)*data_ptr++;
		}
	}
	
	/* If some wrong parameter (or  being passed, don't look for 
	anything. Print default */
	else
	{
		integer_data	=	*data_ptr++;
	}	
	
	integer_data_copy = integer_data;
	
		
	/* MSB first, LSB last */	
	do
	{
		int_char[int_char_index] 	= 	integer_data % divide_factor; 
		
		/* If at all we use divide_factor factor == 16, then we may have to 
		display 'A' 'B' ... 'F' */
		if ((int_char[int_char_index]) >= 10 )
		{
			/* What is happening here? Check ASCII table once */
			int_char[int_char_index] = int_char[int_char_index] - 10 - ZERO_CHAR + \
									   sign_bit - 23;
		}
		int_char[int_char_index] 	= int_char[int_char_index] + ZERO_CHAR;
		
		int_char_index++;
		
		integer_data				=	integer_data / divide_factor;
	}while (integer_data);
	
	/* Let us fill zeros to match the width if user has specified */
	if ((zero_pad -= int_char_index) > 0)
	{
		while (zero_pad--)
		{
			if (1 == use_zero_padding)
			{
				*UART_data_array_ptr++ = ZERO_CHAR;
			}
			else
			{
				*UART_data_array_ptr++ == SPACE;
			}	
		} 
	}
	
	/* Filling in reverse order */
	while (int_char_index--)
	{
		*UART_data_array_ptr++	=	(INT8)int_char[int_char_index];
	}
	
	/* Return the current array pointer */
	return (UART_data_array_ptr);
	
}

/*
* Function Information
*
* Function Name				:		print_float
*
* Function Description		:		This function accepts the float and converts them 
*									into digits, fills there respective ascii values to passed
*									array.
*
* Parameters passed			:		INT8 *UART_data_array_ptr 	: Points to array which contains all 
*									users data
*									INT32 *data_ptr	: Points to the float
*									INT8 zero_pad  : How much zero padding we are doing? Length is 
*									counted including length of precision
*									INT8  precision	: What is the precision expected to display 
*									(decode) ?. Default is 4 and maximum is 9.									 
*
* Parameters returned		:		INT8* UART_data_array_ptr : Pointer, which is pointing to last 
*									updated array location
*
* Functions called			:		print_int
*
* Global variables used		:		None
*
* Global variables modified	:		None
*
*/
static INT8* print_float (INT8 *UART_data_array_ptr, INT32 *data_ptr, INT8 zero_pad, INT8 precision)
{
	f_64		*double_ptr;
	f_64		double_val;
	INT32		truncate_value;
	f_64		decimal_value;
	INT16		index;
	INT32		digit_extracted;		
	
	/* Extract significant and decimal values */
	double_ptr 	= (f_64 *)data_ptr;
	double_val 	= (f_64)*double_ptr;
	
	/* for decimal values we should print ONLY positive */
	if (double_val < ZERO)
	{
		*UART_data_array_ptr++ = '-';
		double_val = -1 * double_val;
		zero_pad--;
	}
	
	truncate_value = (INT32)double_val;
	decimal_value  = double_val - (f_64)truncate_value;
	
	/* Check with standard C compiler. -1 is for '.'*/
	zero_pad = ((zero_pad-precision)-1);
	
	/* We are printing INTEGER part of the float */
	UART_data_array_ptr = print_int (UART_data_array_ptr, &truncate_value, 'd', zero_pad);
	*UART_data_array_ptr++ = '.';
	
	/* Printing decimal places. Precision was passed by user. By default it will 
	be 6 */
	
	for (index = 1; index <= precision; index++)
	{
		decimal_value 	= decimal_value * 10;
		digit_extracted = (INT32)decimal_value;
		decimal_value  	= decimal_value - (f_64)digit_extracted;
		digit_extracted = ZERO_CHAR + digit_extracted;
		
		/* Fill to array */
		*UART_data_array_ptr++ = (INT8)digit_extracted;
		
	}
	
	return(UART_data_array_ptr);

}

