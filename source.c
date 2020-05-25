/* Quora database cooling challenge program
Frederic Spieler
E-mail : fred@fredspieler.com

****************************************************************************************
IMPORTANT!

CONTIGUITY ASSUMPTIONS:
-A room we do not own must either be adjacent to the outside or to another room we do not own
-The starting duct is either next to the outside or next to a room we do not own.

If these assumptions do not hold, this program will produce an incorrect result UNLESS CONT_OPT is disabled (disabled by default)
However, if this assumption does hold, the program will produce the correct result FASTER with CONT_OPT enabled
****************************************************************************************
*/

#define TIMER ON //ON to turn on timer, OFF to turn off
#define CONT_OPT ON  //ON to enable, OFF to disable !!SEE ABOVE!!

/*
Set-up:
******
Includes, constants, globals, and function prototypes
main() inputs from stdin, formats the input, and runs the recursive solver.

It formats a char grid that appears similar to the input grid: 
Start and empty nodes are marked ' '.
Nodes we do not own and border nodes are marked 'X' unless CONT_OPT is disabled, in which case they are marked 'U'. Then, nodes marked 'U' are marked 'X' if they are adjacent to the start node or to another node marked 'X'.
The end node is marked 'E'.

The final result of the input formatting is a grid that is 2 columns and 2 rows larger than the input grid, to allow for border nodes. Every unallowed node that is part of a block of unallowed nodes that is contiguous to the starting node will be marked 'X', all other unallowed nodes will be marked 'U'. 

This section checks for SOME errors in input characters, but ignores spacing.

Recursive solver:
*****************
Without optimizations enabled, this will look for a brute-force solution.
It does this by marking the current space 'X', then calling itself on all adjacent squares not marked 'X'. It then sums the value returned by all of these calls and returns this value after changing the current node value back to ' '.

If CONT_OPT is disabled, the recursive solver also checks if any adjacent nodes are marked 'U'. If so, it calls a function that sets all conitiguous 'U's to 'X's. It then must keep of track of the nodes it performed this operation on so that it can undo the operation before returning.

A counter is incremented and then passed along to the next recursive call. If the node of the current call is 'E' and the count is equal to a final count (corresponding to the total number of nodes an acceptable path), then it returns 1. If the count is less than final count, it retuns 0.

'ELIMINATE' Optimizations:
**************************
The main source of speed-up is two optimization functions that are called during the recursive solver. They recognize patterns that will lead to future calls not returning successful paths. There are explained in more detail later (see eliminate1 and eliminate2). They can be individually enabled and disable using OPTIMIZE1 and OPTIMIZE2. (They can also be reordered, but the current order has been experimentally shown to be best.)

*/

#include <stdio.h>

#define DEBUG OFF //Note: this enables printGrid

//Optimizations: See eliminate function for in-depth explanations
//NOTE: Turn off DEBUG to use optimizations
#define OPTIMIZE1 ON //set ON to optimize using the first fix
#define OPTIMIZE2 ON //set ON to optimize using the second fix 

#define ON 1
#define OFF 0

#if TIMER
#include <stdlib.h>
#include <sys/time.h>
#endif

int rows, cols;
int begrow, begcol, endrow, endcol;
int final_count;

int solver( char grid[rows][cols], int crow, int ccol, int count, int *calls);

#if OPTIMIZE1
inline int eliminate1( char grid[rows][cols], int crow, int ccol );
#endif

#if OPTIMIZE2
inline int eliminate2( char grid[rows][cols], int crow, int ccol );
#endif

#if !CONT_OPT
void u_to_x( char grid[rows][cols], int crow, int ccol );
void x_to_u( char grid[rows][cols], int crow, int ccol );
#endif

#if TIMER
//copied from libc manual: http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
int timeval_subtract ( struct timeval *result, struct timeval *x, struct timeval *y );
#endif

#if DEBUG
void printGrid( char grid[rows][cols] );
#endif

int main()
{
    int calls;
	char input;
	int i,j;
	int sp_exists, ep_exists;
	int ones=1; //how many ones appear in grid? (ones correspond to rooms we don't own)

	#if TIMER
	struct timeval *start, *end, *result;
	start = malloc( sizeof(struct timeval) );
	end = malloc( sizeof(struct timeval) );
	result = malloc( sizeof(struct timeval) );
	#endif

	int successes = 0;
	rows = 0;
	cols = 0;
	sp_exists = 0;
	ep_exists = 0;

//HANDLE INPUTS

	//get rows and cols (arbitrary number of digits)	
	while( (input = getchar()) != ' ')
	{
		cols = cols * 10;
		cols += atoi( &input );
	}
	cols += 2; //borders
	while( (input = getchar()) != '\n')
	{
		rows = rows * 10;
		rows += atoi( &input );
	}
	rows += 2; //borders

	char grid[rows][cols];
	
	//fill grid with initial values
	for( i = 0; i < rows; i++ )
	{
		for( j = 0; j < cols; j++ )
		{
			if( i == 0 || i == rows-1 || j == 0 || j == cols-1 )
			{
				#if CONT_OPT
				grid[i][j] = 'X';
				#else
				grid[i][j] = 'U';
				#endif
	 		}
			else{
				while( (input = getchar() ) == ' ' || input == '\n' );
				if( input == '0' )
				{
					grid[i][j] = ' ';
				}
				else if( input == '1' )
				{
					ones++;
					#if CONT_OPT
					grid[i][j] = 'X';
					#else
					grid[i][j] = 'U';
					#endif
				}
				else if( input == '2' )
				{
					begrow = i;
					begcol = j;
					grid[i][j] = 'S';
					if( !sp_exists )
						sp_exists++;
					else
					{
						printf("Error: Only one starting point allowed!\n");
						return;
					}
				}
				else if( input == '3' )
				{
					endrow = i;
					endcol = j;
					grid[i][j] = 'E';
					if( !ep_exists )
						ep_exists++;
					else
					{
						printf("Error: Only one ending point allowed!\n");
						return;
					}
				}
				else
				{
					printf("Error: Strange character: \"%c\" \n", input );
					return;
				}
			}
		}
	}

	if( !sp_exists && 0 )
	{
		printf("Error: No starting point found!\n");
		return;
	}
	if( !ep_exists && 0 )
	{
		printf("Error: No ending point found!\n");
		return;
	}

//RUN RECURSIVE FUNCTION
	#if TIMER
	gettimeofday(start, NULL);
	#endif

	#if !CONT_OPT
	//left
	if( grid[begrow][begcol-1] == 'U' )
		u_to_x( grid, begrow, begcol-1 );
	//down
	if( grid[begrow+1][begcol] == 'U' )
		u_to_x( grid, begrow+1, begcol );
	//right
	if( grid[begrow][begcol+1] == 'U' )
		u_to_x( grid, begrow, begcol+1 );
	//up
	if( grid[begrow-1][begcol] == 'U' )
		u_to_x( grid, begrow-1, begcol );
	#endif
	
	final_count = ((rows-2)*(cols-2)) - ones;

    calls = 0;
	successes = solver( grid, begrow, begcol, 0, &calls);
    printf("calls: %d\n", calls);

	printf("%d\n", successes);

	#if TIMER
	gettimeofday(end, NULL);
	
	if( timeval_subtract( result, end, start ) < 0 )
		printf("Error with timing function :(\n");
	else
		printf("elapsed: %ld.%06ld seconds\n", result->tv_sec, result->tv_usec );
	#endif
}

//recursive solver
int solver( char grid[rows][cols], int crow, int ccol, int count, int *calls )
{
    (*calls)++;
	int i, j;
	int successes = 0;

	#if !CONT_OPT
	//these are necessary for keeping track of which cells the u_to_x function has been applied so that the reverse function can be applied prior to return
	int uleft = 0;
	int udown = 0;
	int uright = 0;
	int uup = 0;
	
	#endif

	//Check if reached the end
	if( grid[crow][ccol] == 'E' )
	{
		if( count == final_count )
		{
			return 1;
		}
		return 0;
	}

	#if OPTIMIZE1
	if( eliminate1( grid, crow, ccol ) )
		return 0;
	#endif

	#if !CONT_OPT
	grid[crow][ccol] = 'X';
	//left
	if( grid[crow][ccol-1] == 'U' )
	{
		u_to_x( grid, crow, ccol-1 );
		uleft = 1;
	}
	//down
	if( grid[crow+1][ccol] == 'U' )
	{
		u_to_x( grid, crow+1, ccol );
		udown = 1;
	}
	//right
	if( grid[crow][ccol+1] == 'U' )
	{
		u_to_x( grid, crow, ccol+1 );
		uright = 1;
	}
	//up
	if( grid[crow-1][ccol] == 'U' )
	{
		u_to_x( grid, crow-1, ccol );
		uup = 1;
	}
	#endif

	#if OPTIMIZE2
	if( eliminate2( grid, crow, ccol ) )
		#if CONT_OPT
		return 0;
		#else
	{
		grid[crow][ccol] = ' ';
		//left
		if( uleft )
		{
			x_to_u( grid, crow, ccol-1 );
		}
		//down
		if( udown )
		{
			x_to_u( grid, crow+1, ccol );
		}
		//right
		if( uright )
		{
			x_to_u( grid, crow, ccol+1 );
		}
		//up
		if( uup )
		{
			x_to_u( grid, crow-1, ccol );
		}
		return 0;
	}		
		#endif
	#endif
	#if CONT_OPT
	grid[crow][ccol] = 'X';
	#endif

	//left
	if( grid[crow][ccol-1] != 'X' )
		successes += solver( grid, crow, ccol-1, count + 1, calls );
	//down
	if( grid[crow+1][ccol] != 'X' )
		successes += solver( grid, crow+1, ccol, count + 1, calls );
	//right
	if( grid[crow][ccol+1] != 'X' )
		successes += solver( grid, crow, ccol+1, count + 1, calls ); 
	//up
	if( grid[crow-1][ccol] != 'X' )
		successes += solver( grid, crow-1, ccol, count + 1, calls );
	
	grid[crow][ccol] = ' ';

	#if !CONT_OPT
	//left
	if( uleft )
	{
		x_to_u( grid, crow, ccol-1 );
	}
	//down
	if( udown )
	{
		x_to_u( grid, crow+1, ccol );
	}
	//right
	if( uright )
	{
		x_to_u( grid, crow, ccol+1 );
	}
	//up
	if( uup )
	{
		x_to_u( grid, crow-1, ccol );
	}
	#endif

	return successes;
}

#if DEBUG
void printGrid( char grid[rows][cols] )
{
	int i, j;
	for( i = 0; i < rows; i++ )
	{	
		printf("|");
		for( j = 0; j < cols; j++ )
		{
				printf("%c|", grid[i][j]);

		}
		printf("\n");
	}
	printf("\n");
}
#endif

#if OPTIMIZE1
/*Returns 1 if branch can be eliminated
This optimization checks to see if the last move created two non-contiguous regions.
The following scenarios should lead to losses and should return immediately (further branches are not necessary):

|*|  |_|
_X_  *X*
|*|  |_|

Where * is an occupied, ineligible space, _ is a vacant space, X is the current space, and | is Do Not Care.

*/
inline int eliminate1( char grid[rows][cols], int crow, int ccol )
{
	#if CONT_OPT
	if( grid[crow-1][ccol] != 'X' && grid[crow+1][ccol] != 'X' && grid[crow][ccol-1] == 'X' && grid[crow][ccol+1] == 'X' )
		return 1;
	if( grid[crow][ccol-1] != 'X' && grid[crow][ccol+1] != 'X' && grid[crow-1][ccol] == 'X' && grid[crow+1][ccol] == 'X' )
		return 1;
	#else
	if( grid[crow-1][ccol] != 'X' && grid[crow+1][ccol] != 'X' && grid[crow-1][ccol] != 'U' && grid[crow+1][ccol] != 'U' && grid[crow][ccol-1] == 'X' && grid[crow][ccol+1] == 'X' )
		return 1;
	if( grid[crow][ccol-1] != 'X' && grid[crow][ccol+1] != 'X' && grid[crow][ccol-1] != 'X' && grid[crow][ccol+1] != 'X' && grid[crow-1][ccol] == 'X' && grid[crow+1][ccol] == 'X' )
		return 1;
	#endif
	
	
	return 0;
}
#endif

#if OPTIMIZE2
/* Returns 1 if branch can be eliminated
This optimization checks to see if the last move created two non-contiguous regions.
This checks if the boundary created is diagonal.
The following scenarios should lead to losses and should return immediately (further branches are not necessary):

TR  TL  BL  BR
_*  *_  _X  X_
X_  _X  *_  _*

where * is an occupied or ineligible space, _ is a vacant space, X is the current space, and | is Do Not Care.
 */
inline int eliminate2( char grid[rows][cols], int crow, int ccol )
{
	//TR
	if( grid[crow-1][ccol] != 'X' && grid[crow][ccol+1] != 'X' && grid[crow-1][ccol+1] == 'X' )
		return 1;
	//TL
	if( grid[crow-1][ccol] != 'X' && grid[crow][ccol-1] != 'X' && grid[crow-1][ccol-1] == 'X' )
		return 1;
	//BL
	if( grid[crow+1][ccol] != 'X' && grid[crow][ccol-1] != 'X' && grid[crow+1][ccol-1] == 'X' )
		return 1;
	//BR
	if( grid[crow+1][ccol] != 'X' && grid[crow][ccol+1] != 'X' && grid[crow+1][ccol+1] == 'X' )
		return 1;
	return 0;
}
#endif

#if !CONT_OPT
void u_to_x( char grid[rows][cols], int crow, int ccol )
{
	if( crow < 0 || ccol < 0 || crow >= rows || ccol >= cols )
		return;

	grid[crow][ccol] = 'X';
	//left
	if( grid[crow][ccol-1] == 'U' )
		u_to_x( grid, crow, ccol-1 );
	//down
	if( grid[crow+1][ccol] == 'U' )
		u_to_x( grid, crow+1, ccol );
	//right
	if( grid[crow][ccol+1] == 'U' )
		u_to_x( grid, crow, ccol+1 );
	//up
	if( grid[crow-1][ccol] == 'U' )
		u_to_x( grid, crow-1, ccol );
	
	return;
}

void x_to_u( char grid[rows][cols], int crow, int ccol )
{
	if( crow < 0 || ccol < 0 || crow >= rows || ccol >= cols )
		return;

	grid[crow][ccol] = 'U';


	//left
	if( grid[crow][ccol-1] == 'X' )
		x_to_u( grid, crow, ccol-1 );
	//down
	if( grid[crow+1][ccol] == 'X' )
		x_to_u( grid, crow+1, ccol );
	//right
	if( grid[crow][ccol+1] == 'X' )
		x_to_u( grid, crow, ccol+1 );
	//up
	if( grid[crow-1][ccol] == 'X' )
		x_to_u( grid, crow-1, ccol );
	
	return;
}
#endif

#if TIMER
//copied from libc manual: http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
int timeval_subtract ( struct timeval *result, struct timeval *x, struct timeval *y )
{

  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}
#endif
