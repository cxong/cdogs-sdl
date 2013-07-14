#ifndef _XGETOPT_H
#define _XGETOPT_H

/*
  getopt.h - comand line option parsing
  Copyright Keristor Systems and Chris Croughton 1997 - 2005
  Internet: swdev@keristor.org

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

/**@file xgetopt.h
 *
 * This module is designed as a "drop-in" replacement for the GNU
 *  \c getopt.h module, providing the  \c getopt(),  \c getopt_long() and
 *  \c getopt_long_only() functions and the  \c option structure.  However,
 * it is a compile-time replacement not an object one, the actual
 * names of the external variables and functions are hidden so that
 * namespace clashes don't occur (some linkers object if they find
 * the same symbol in more than one library).
 *
 * It was written originally because of systems which don't have the GNU
 * C library installed.
 *
 * It also provides a few functions to help create a string of short
 * (single-character) options given an array of long option structures.
 *
 * This is a "clean-room" implementation, no GPL or proprietary code
 * was looked at when creating it.  The only things used were the
 * descriptions of the interfaces in the  \c man pages (and the POSIX
 * specification), and some "black box" testing to determine error
 * conditions and messages not specified in the interface description.
 */

#if defined(_cplusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * This bit of faffing about is because some systems don't like the same
 * functions defined in more than one library, they get confused.  So what
 * we do is to redefine the variables to the names in the interface but
 * fool the linker.  This only makes a difference if you try to debug the
 * getopt code.  Note that we don't have to do it with the structure name
 * or enums, since it's only linkage that's affected.
 */

#define optarg                 _XLIB_optarg
#define optind                 _XLIB_optind
#define opterr                 _XLIB_opterr
#define optopt                 _XLIB_optopt

#define getopt                 _XLIB_getopt
#define getopt_long            _XLIB_getopt_long
#define getopt_long_only       _XLIB_getopt_long_only

/** holds a pointer to an option argument */
extern char *optarg;
/** index to te first non-option argument */
extern int   optind;
/** allow error messages if non-zero (default) */
extern int   opterr;
/** holds the option character in error, if any */
extern int   optopt;

enum
{
  /** this option has no argument */
  no_argument,
  /** an argument is required with this option */
  required_argument,
  /** an argument to this option is optional */
  optional_argument
};

/**
 * Structure to describe a long option.
 * The last item in the array of these descriptors must have all elements
 * set to zero
 */
struct option
{
  /** long option name */
  const char *name;
  /** argument type, if any */
  int         has_arg;
  /** pointer to a flag to be set if the option is present */
  int        *flag;
  /** value to be returned if the option is present */
  int         val;
};

/** Scan the command-line parameters for options in the form {\em -x}. */
int getopt(int argc, char * argv[], const char *optstring);
/** Scan the command-line parameters for options, allowing both the short
 * (single character) options and long (string) options.
 */
int getopt_long(int argc, char * argv[], const char *optstring,
                const struct option *longopts, int *longindex);
/** Scan the command-line parameters for long options only */
int getopt_long_only(int argc, char * argv[], const char *optstring,
                     const struct option *longopts, int *longindex);

#if defined(_cplusplus) || defined(__cplusplus)
}
#endif

#endif
