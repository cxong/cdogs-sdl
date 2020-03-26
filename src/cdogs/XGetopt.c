/*
  getopt.c - implementation of getopt(3) and getopt_long(3)
  Copyright Keristor Systems and Chris Croughton 1997
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
#ifdef _WIN32

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define XGETOPT_NAMES
#include "XGetopt.h"

/*
 * This is the published interface for the GNU getopt module
 */

char *optarg = NULL;
int   optind = 0;
int   opterr = 1;
int   optopt = 0;

/************************************************************************
 *
 * Everything from here down is a rewrite without looking at the GNU
 * implementation; it is therefore free of GPL and LGPL contraints, see
 * above for licencing information.
 *
 ************************************************************************/

#define true  1
#define false 0
#define bool  char

enum action
{
    E_PERMUTE,
    E_POSIX,
    E_INPLACE,
    E_NUM_OF_ACTIONS
};

static bool  initialised = false;
static bool  endargs     = false;
static char *nextchar    = NULL;

/* Note that the lookup routines use and modify the global variables - not
 * nice, but better than passing everything as reference parameters.
 */

static int
lookup_shortopt(int argc, char * const *argv, const char *opts)
{
    int c = *optarg++;
    /* remember the Alamo - a dodgy character if ever there was one */
    nextchar = optarg;
    /* look at the options - like any you see? */
    while (*opts)
    {
        if (c == *opts++)
        {
            /* OK, it's a fair (c)op, guv */
            if (*opts == ':')
            {
                /* we can do arguments, but there will be nothing left of this arg */
                nextchar = NULL;
                if (*++opts == ':')
                {
                    /* but it's optional, so either a 5 minute argument or nothing */
                    if (!*optarg)
                        optarg = NULL;
                    return c;
                }
                else
                {
                    /* this argument's not optional, you get the full half hour */
                    if (*optarg)
                    {
                        /* We've got an argument on our hands! */
                        return c;
                    }
                    else
                    {
                        /* try the next door down */
                        if (++optind < argc)
                        {
                            /* OK, we've got it ("Oh no you haven't!") */
                            optarg = argv[optind];
                            return c;
                        }
                        else
                        {
                            /* Th-Th-Th-That's All, Folks! */
                            optopt = c;
                            if (opterr)
                                fprintf(stderr, "required parameter for -%c missing\n", c);
                            /* we've got an option, but it's dud */
                            return '?';
                        }
                    }
                }
            }
            else
            {
                /* no arguments please, we're British */
                nextchar = optarg;
                optarg = NULL;
                return c;
            }
        }
    }
    /* return to sender, address unknown */
    optopt = c;
    optarg = NULL;
    return -1;
}

static int lookup_longopt(
	int argc, char *const *argv,
               const struct option *longopts,
               int *longindex)
{
    char *eqp;
    bool  ambig = false;
    int   match = -1;
    int   exact = -1;
    int   min;
    int   len;
    int   i;

    /* if we have no long options, any we get are wrong */
    if (!longopts)
    {
        if (opterr)
            fprintf(stderr, "long options not supported\n");
        return '?';
    }
    /* look for equality or fraternity */
    eqp = strchr(optarg, '=');
    min = (eqp ? (int)(eqp - optarg) : (int)strlen(optarg));
    for (i = 0; longopts[i].name; i++)
    {
        len = (int)strlen(longopts[i].name);
        /* is it exact? */
        if (len == min && strncmp(longopts[i].name, optarg, len) == 0)
        {
            /* an exact match - go no further, we can't get better than this */
            exact = i;
            break;
        }
        /* a match as far as the argument goes? */
        if (strncmp(longopts[i].name, optarg, min) == 0)
        {
            /* if we've already found one of those, it's ambiguous */
            if (match < 0)
                match = i;
            else
                ambig = true;
        }
    }
    /* we've looked at everything, now what? */
    if (exact < 0)
    {
        /* no exact match, if it's ambiguous then fail othersise use what
         * (if anything) we have */
        if (ambig)
        {
            if (opterr)
                fprintf(stderr, "option --%s is ambiguous\n", optarg);
            return '?';
        }
        else
            exact = match;
    }
    /* if we don't have anything, return -1 (/not/ fail, this could be a test */
    if (exact < 0)
        return -1;
    switch (longopts[exact].has_arg)
    {
        case no_argument:
            /* we have nothing to fight about... */
            if (eqp)
            {
                /* ... except equality */
                if (opterr)
                    fprintf(stderr, "option --%s does not take an argument\n", optarg);
                return '?';
            }
            optarg = NULL;
            break;
        case required_argument:
            /* you have to fight, so there! */
            if (eqp)
                optarg = eqp + 1;
            else if (++optind < argc)
                optarg = argv[optind];
            else
            {
                if (opterr)
                    fprintf(stderr, "missing argument for option --%s\n", optarg);
                return '?';
            }
            break;
        case optional_argument:
            /* you don't have to fight if you don't want to */
            optarg = (eqp ? eqp + 1 : NULL);
            break;
    }
    /* tell the caller what he's got */
    if (longindex)
        *longindex = exact;
    if (longopts[exact].flag)
    {
        /* set a flag rather than returning the value */
        *longopts[exact].flag = longopts[exact].val;
        return 0;
    }
    return longopts[exact].val;
}

/*
 * Note that this internal function drops the const on the arg list.  That's
 * because optarg etc. are defined without const.
 */
static int getnextopt(
	int argc, char *const *argv, const char *optstring,
           const struct option *longopts, int *longindex,
           bool longonly, enum action act)
{
    static int thisind = 0;
    static int oldind = 0;
    int c = 0;

    /* if we haven't started, make sure we're not in Denmark */
    if (!initialised)
    {
        initialised = true;
        nextchar    = NULL;
        endargs     = false;
        thisind     = 0;
        oldind      = 0;
        optind      = 0;
    }

    /* check for previous sequence of short options */
    if (nextchar && *nextchar)
    {
        optarg = nextchar;
        c = lookup_shortopt(argc, argv, optstring);
        /* now shuffle the arguments if necessary */
        if (act == E_PERMUTE && oldind != thisind && (!nextchar || !*nextchar))
        {
            int optlen = optind - thisind; /* the length of the option */
            int optnum = thisind - oldind; /* number of args in between */
            memmove(argv+oldind+optlen+1, argv+oldind, sizeof(*argv) * optnum);
            optind = oldind + optlen;
        }
        return c;
    }

    /* check for end of argument list */
    if (++optind >= argc || !argv[optind])
    {
        optarg = NULL;
        return EOF;        /* no arguments left */
    }

    /* get next argument from list */
    optarg = argv[optind];

    /* if we've had "--" then treat as normal argument */
    if (endargs)
        return (act == E_INPLACE ? '\1' : EOF);

    /* if we're permuting the arguments, look for the next option */
    if (act == E_PERMUTE)
    {
        int i;
        /* save where we are, we'll need this later */
        oldind = optind;
        for (i = optind; i < argc; i++)
        {
            char *p = argv[i];
            /* remember that "--" ends options?  Ignore anything past it... */
            if (strcmp(p, "--") == 0)
            {
                int optnum = i - oldind; /* number of args in between */
                if (optnum > 0)
                {
                    /* shuffle the arguments up over it */
                    memmove(argv+oldind+1, argv+oldind, sizeof(*argv) * optnum);
                    optind = ++oldind;
                    optarg = argv[optind];
                }
                break;
            }
            if (*p == '-' && strcmp(p, "-"))
            {
                /* we found a honest-to-goodness option!  Set up shop here... */
                optind = i;
                optarg = p;
                break;
            }
        }
    }

    /* test for "--" meaning "end of options" - everything after that is an
     * argument not an option */
    if (strcmp(optarg, "--") == 0)
    {
        endargs = true;
        if (++optind < argc)
        {
            optarg = argv[optind];
            return (act == E_INPLACE ? '\1' : EOF);
        }
        else
        {
            optarg = NULL;
            return EOF;
        }
    }

    /* an argument of "-" or not starting with '-'is not an option */
    if (strcmp(optarg, "-") == 0 || *optarg != '-')
    {
        /* if we're being POSIXLY_CORRECT (ugh!) then the first non-option ends
         * the options.  I don't like it but that's what the spec. says... */
        if (act == E_POSIX)
            endargs = true;
        return (act == E_INPLACE ? '\1' : EOF);
    }

    thisind = optind; /* remember this position, lookup changes it */

    /* OK, it's some sort of option, junk the first '-' */
    optarg++;

    /* test the next character - another '-' means it's a long option */
    if (longopts && *optarg == '-')
    {
        /* it's a long option, skip the next '-' */
        optarg++;
        c = lookup_longopt(argc, argv, longopts, longindex);
        if (c < 0)
        {
            fprintf(stderr, "unknown option --%s\n", optarg);
            c = '?';
        }
    }
    else if (longopts && longonly)
    {
        int tmperr = opterr;
        opterr = 0;
        /* try a long option first, if that doesn't work try a short one */
        c = lookup_longopt(argc, argv, longopts, longindex);
        opterr = tmperr;
        if (c < 0)
            c = lookup_shortopt(argc, argv, optstring);
    }
    else
    {
        /* just try short options */
        c = lookup_shortopt(argc, argv, optstring);
    }

    /* if not found give (optional) error and return '?' */
    if (c < 0)
    {
        if (opterr)
            fprintf(stderr, "unknown option -%c\n", optopt);
        optarg = 0;
        c = '?';
    }

    /* now shuffle the arguments if necessary - I told you we'd need to save
     * that old index!  (I wonder if there's a more elegant way to do this?) */
    if (act == E_PERMUTE && oldind != thisind && (!nextchar || !*nextchar))
    {
        int optlen = optind - thisind; /* the length of the option */
        int optnum = thisind - oldind; /* number of args in between */
        memmove(argv+oldind+optlen+1, argv+oldind, sizeof(*argv) * optnum);
        optind = oldind + optlen;
        oldind = 0;
        thisind = 0;
    }
    return c;
}


/***********************************************************************
 * At last, the real routines we wanted to call!
 **********************************************************************/

/**
 * Scan the command-line parameters for options in the form \b -x.
 */
int getopt(int argc, char * const argv[], const char *optstring)
{
	return getnextopt(
		argc, (char *const *)argv, optstring, NULL, NULL,
                      false, E_PERMUTE);
}

/**
 * Scan the command-line parameters for options, allowing both the short
 * (single character) options and long (string) options.
 */
int getopt_long(int argc, char * argv[], const char *optstring,
                const struct option *longopts, int *longindex)
{
	return getnextopt(
		argc, (char *const *)argv, optstring, longopts, longindex,
                      false, E_PERMUTE);
}

/**
 * Scan the command-line parameters for long options only
 */
int getopt_long_only(int argc, char * argv[], const char *optstring,
                     const struct option *longopts, int *longindex)
{
	return getnextopt(
		argc, (char *const *)argv, optstring, longopts, longindex,
                      true, E_PERMUTE);
}

#endif
