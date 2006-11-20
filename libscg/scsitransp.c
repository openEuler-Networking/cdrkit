/*
 * This file has been modified for the cdrkit suite.
 *
 * The behaviour and appearence of the program code below can differ to a major
 * extent from the version distributed by the original author(s).
 *
 * For details, see Changelog file distributed with the cdrkit package. If you
 * received this file from another source then ask the distributing person for
 * a log of modifications.
 *
 */

/* @(#)scsitransp.c	1.91 04/06/17 Copyright 1988,1995,2000-2004 J. Schilling */
/*#ifndef lint*/
static	char sccsid[] =
	"@(#)scsitransp.c	1.91 04/06/17 Copyright 1988,1995,2000-2004 J. Schilling";
/*#endif*/
/*
 *	SCSI user level command transport routines (generic part).
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1988,1995,2000-2004 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <mconfig.h>
#include <stdio.h>
#include <standard.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <errno.h>
#include <timedefs.h>
#include <strdefs.h>
#include <schily.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>
#include "scgtimes.h"

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
static	char	_scg_version[]		= "0.8ubuntu1+debburn1";	/* The global libscg version	*/
static	char	_scg_auth_ubuntu[]	= "debburn project";	/* The author for this module	*/

#define	DEFTIMEOUT	20	/* Default timeout for SCSI command transport */

char	*scg_version(SCSI *scgp, int what);
int	scg__open(SCSI *scgp, char *device);
int	scg__close(SCSI *scgp);
BOOL	scg_havebus(SCSI *scgp, int);
int	scg_initiator_id(SCSI *scgp);
int	scg_isatapi(SCSI *scgp);
int	scg_reset(SCSI *scgp, int what);
void	*scg_getbuf(SCSI *scgp, long);
void	scg_freebuf(SCSI *scgp);
long	scg_bufsize(SCSI *scgp, long);
void	scg_setnonstderrs(SCSI *scgp, const char **);
BOOL	scg_yes(char *);
#ifdef	nonono
static	void	scg_sighandler(int);
#endif
int	scg_cmd(SCSI *scgp);
void	scg_vhead(SCSI *scgp);
int	scg_svhead(SCSI *scgp, char *buf, int maxcnt);
int	scg_vtail(SCSI *scgp);
int	scg_svtail(SCSI *scgp, int *retp, char *buf, int maxcnt);
void	scg_vsetup(SCSI *scgp);
int	scg_getresid(SCSI *scgp);
int	scg_getdmacnt(SCSI *scgp);
BOOL	scg_cmd_err(SCSI *scgp);
void	scg_printerr(SCSI *scgp);
void	scg_fprinterr(SCSI *scgp, FILE *f);
int	scg_sprinterr(SCSI *scgp, char *buf, int maxcnt);
int	scg__sprinterr(SCSI *scgp, char *buf, int maxcnt);
void	scg_printcdb(SCSI *scgp);
int	scg_sprintcdb(SCSI *scgp, char *buf, int maxcnt);
void	scg_printwdata(SCSI *scgp);
int	scg_sprintwdata(SCSI *scgp, char *buf, int maxcnt);
void	scg_printrdata(SCSI *scgp);
int	scg_sprintrdata(SCSI *scgp, char *buf, int maxcnt);
void	scg_printresult(SCSI *scgp);
int	scg_sprintresult(SCSI *scgp, char *buf, int maxcnt);
void	scg_printstatus(SCSI *scgp);
int	scg_sprintstatus(SCSI *scgp, char *buf, int maxcnt);
void	scg_fprbytes(FILE *, char *, unsigned char *, int);
void	scg_fprascii(FILE *, char *, unsigned char *, int);
void	scg_prbytes(char *, unsigned char *, int);
void	scg_prascii(char *, unsigned char *, int);
int	scg_sprbytes(char *buf, int maxcnt, char *, unsigned char *, int);
int	scg_sprascii(char *buf, int maxcnt, char *, unsigned char *, int);
void	scg_fprsense(FILE *f, unsigned char *, int);
int	scg_sprsense(char *buf, int maxcnt, unsigned char *, int);
void	scg_prsense(unsigned char *, int);
int	scg_cmd_status(SCSI *scgp);
int	scg_sense_key(SCSI *scgp);
int	scg_sense_code(SCSI *scgp);
int	scg_sense_qual(SCSI *scgp);
unsigned char *scg_sense_table(SCSI *scgp);
void	scg_fprintdev(FILE *, struct scsi_inquiry *);
void	scg_printdev(struct scsi_inquiry *);
int	scg_printf(SCSI *scgp, const char *form, ...);
int	scg_errflush(SCSI *scgp);
int	scg_errfflush(SCSI *scgp, FILE *f);

/*
 * Return version information for the SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 *
 * If scgp is NULL, return general library version information.
 * If scgp is != NULL, return version information for the low level transport.
 */
char *
scg_version(SCSI *scgp, int what)
{
	if (scgp == (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
			return (_scg_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
			return (_scg_auth_ubuntu);
		case SCG_SCCS_ID:
			return (sccsid);
		default:
			return ((char *)0);
		}
	}
	return (SCGO_VERSION(scgp, what));
}

/*
 * Call low level SCSI open routine from transport abstraction layer.
 */
int
scg__open(SCSI *scgp, char *device)
{
	int	ret;
	scg_ops_t *ops;
extern	scg_ops_t scg_std_ops;

	scgp->ops = &scg_std_ops;

	if (device && strncmp(device, "REMOTE", 6) == 0) {
		ops = scg_remote();
		if (ops != NULL)
			scgp->ops = ops;
	}

	ret = SCGO_OPEN(scgp, device);
	if (ret < 0)
		return (ret);

	/*
	 * Now make scgp->fd valid if possible.
	 * Note that scg_scsibus(scgp)/scg_target(scgp)/scg_lun(scgp) may have
	 * changed in SCGO_OPEN().
	 */
	scg_settarget(scgp, scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
	return (ret);
}

/*
 * Call low level SCSI close routine from transport abstraction layer.
 */
int
scg__close(SCSI *scgp)
{
	return (SCGO_CLOSE(scgp));
}

/*
 * Retrieve max DMA count for this target.
 */
long
scg_bufsize(SCSI *scgp, long amt)
{
	long	maxdma;

	maxdma = SCGO_MAXDMA(scgp, amt);
	if (amt <= 0 || amt > maxdma)
		amt = maxdma;

	scgp->maxdma = maxdma;	/* Max possible  */
	scgp->maxbuf = amt;	/* Current value */

	return (amt);
}

/*
 * Allocate a buffer that may be used for DMA.
 */
void *
scg_getbuf(SCSI *scgp, long amt)
{
	void	*buf;

	if (amt <= 0 || amt > scg_bufsize(scgp, amt))
		return ((void *)0);

	buf = SCGO_GETBUF(scgp, amt);
	scgp->bufptr = buf;
	return (buf);
}

/*
 * Free DMA buffer.
 */
void
scg_freebuf(SCSI *scgp)
{
	SCGO_FREEBUF(scgp);
	scgp->bufptr = NULL;
}

/*
 * Check if 'busno' is a valid SCSI bus number.
 */
BOOL
scg_havebus(SCSI *scgp, int busno)
{
	return (SCGO_HAVEBUS(scgp, busno));
}

/*
 * Return SCSI initiator ID for current SCSI bus if available.
 */
int
scg_initiator_id(SCSI *scgp)
{
	return (SCGO_INITIATOR_ID(scgp));
}

/*
 * Return a hint whether current SCSI target refers to a ATAPI device.
 */
int
scg_isatapi(SCSI *scgp)
{
	return (SCGO_ISATAPI(scgp));
}

/*
 * Reset SCSI bus or target.
 */
int
scg_reset(SCSI *scgp, int what)
{
	return (SCGO_RESET(scgp, what));
}

/*
 * Set up nonstd error vector for curren target.
 * To clear additional error table, call scg_setnonstderrs(scgp, NULL);
 * Note: do not use this when scanning the SCSI bus.
 */
void
scg_setnonstderrs(SCSI *scgp, const char **vec)
{
	scgp->nonstderrs = vec;
}

/*
 * Simple Yes/No answer checker.
 */
BOOL
scg_yes(char *msg)
{
	char okbuf[10];

	js_printf("%s", msg);
	flush();
	if (getline(okbuf, sizeof (okbuf)) == EOF)
		exit(EX_BAD);
	if (streql(okbuf, "y") || streql(okbuf, "yes") ||
	    streql(okbuf, "Y") || streql(okbuf, "YES"))
		return (TRUE);
	else
		return (FALSE);
}

#ifdef	nonono
static void
scg_sighandler(int sig)
{
	js_printf("\n");
	if (scsi_running) {
		js_printf("Running command: %s\n", scsi_command);
		js_printf("Resetting SCSI - Bus.\n");
		if (scg_reset(scgp) < 0)
			errmsg("Cannot reset SCSI - Bus.\n");
	}
	if (scg_yes("EXIT ? "))
		exit(sig);
}
#endif

/*
 * Send a SCSI command.
 * Do error checking and reporting depending on the values of
 * scgp->verbose, scgp->debug and scgp->silent.
 */
int
scg_cmd(SCSI *scgp)
{
		int		ret;
	register struct	scg_cmd	*scmd = scgp->scmd;

	/*
	 * Reset old error messages in scgp->errstr
	 */
	scgp->errptr = scgp->errbeg = scgp->errstr;

	scmd->kdebug = scgp->kdebug;
	if (scmd->timeout == 0 || scmd->timeout < scgp->deftimeout)
		scmd->timeout = scgp->deftimeout;
	if (scgp->disre_disable)
		scmd->flags &= ~SCG_DISRE_ENA;
	if (scgp->noparity)
		scmd->flags |= SCG_NOPARITY;

	scmd->u_sense.cmd_sense[0] = 0;		/* Paranioa */
	if (scmd->sense_len > SCG_MAX_SENSE)
		scmd->sense_len = SCG_MAX_SENSE;
	else if (scmd->sense_len < 0)
		scmd->sense_len = 0;

	if (scgp->verbose) {
		scg_vhead(scgp);
		scg_errflush(scgp);
	}

	if (scgp->running) {
		if (scgp->curcmdname) {
			fprintf(stderr, "Currently running '%s' command.\n",
							scgp->curcmdname);
		}
		raisecond("SCSI ALREADY RUNNING !!", 0L);
	}
	scgp->cb_fun = NULL;
	gettimeofday(scgp->cmdstart, (struct timezone *)0);
	scgp->curcmdname = scgp->cmdname;
	scgp->running = TRUE;
	ret = SCGO_SEND(scgp);
	scgp->running = FALSE;
	__scg_times(scgp);
	if (ret < 0) {
		/*
		 * Old /dev/scg versions will not allow to access targets > 7.
		 * Include a workaround to make this non fatal.
		 */
		if (scg_target(scgp) < 8 || geterrno() != EINVAL)
			comerr("Cannot send SCSI cmd via ioctl\n");
		if (scmd->ux_errno == 0)
			scmd->ux_errno = geterrno();
		if (scmd->error == SCG_NO_ERROR)
			scmd->error = SCG_FATAL;
		if (scgp->debug > 0) {
			errmsg("ret < 0 errno: %d ux_errno: %d error: %d\n",
					geterrno(), scmd->ux_errno, scmd->error);
		}
	}

	ret = scg_vtail(scgp);
	scg_errflush(scgp);
	if (scgp->cb_fun != NULL)
		(*scgp->cb_fun)(scgp->cb_arg);
	return (ret);
}

/*
 * Fill the head of verbose printing into the SCSI error buffer.
 * Action depends on SCSI verbose status.
 */
void
scg_vhead(SCSI *scgp)
{
	scgp->errptr += scg_svhead(scgp, scgp->errptr, scg_errrsize(scgp));
}

/*
 * Fill the head of verbose printing into a buffer.
 * Action depends on SCSI verbose status.
 */
int
scg_svhead(SCSI *scgp, char *buf, int maxcnt)
{
	register char	*p = buf;
	register int	amt;

	if (scgp->verbose <= 0)
		return (0);

	amt = js_snprintf(p, maxcnt,
		"\nExecuting '%s' command on Bus %d Target %d, Lun %d timeout %ds\n",
								/* XXX Really this ??? */
/*		scgp->cmdname, scg_scsibus(scgp), scg_target(scgp), scgp->scmd->cdb.g0_cdb.lun,*/
		scgp->cmdname, scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp),
		scgp->scmd->timeout);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	amt = scg_sprintcdb(scgp, p, maxcnt);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	if (scgp->verbose > 1) {
		amt = scg_sprintwdata(scgp, p, maxcnt);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	return (p - buf);
}

/*
 * Fill the tail of verbose printing into the SCSI error buffer.
 * Action depends on SCSI verbose status.
 */
int
scg_vtail(SCSI *scgp)
{
	int	ret;

	scgp->errptr += scg_svtail(scgp, &ret, scgp->errptr, scg_errrsize(scgp));
	return (ret);
}

/*
 * Fill the tail of verbose printing into a buffer.
 * Action depends on SCSI verbose status.
 */
int
scg_svtail(SCSI *scgp, int *retp, char *buf, int maxcnt)
{
	register char	*p = buf;
	register int	amt;
	int	ret;

	ret = scg_cmd_err(scgp) ? -1 : 0;
	if (retp)
		*retp = ret;
	if (ret) {
		if (scgp->silent <= 0 || scgp->verbose) {
			amt = scg__sprinterr(scgp, p, maxcnt);
			if (amt < 0)
				return (amt);
			p += amt;
			maxcnt -= amt;
		}
	}
	if ((scgp->silent <= 0 || scgp->verbose) && scgp->scmd->resid) {
		if (scgp->scmd->resid < 0) {
			/*
			 * An operating system that does DMA the right way
			 * will not allow DMA overruns - it will stop DMA
			 * before bad things happen.
			 * A DMA residual count < 0 (-1) is a hint for a DMA
			 * overrun but does not affect the transfer count.
			 */
			amt = js_snprintf(p, maxcnt, "DMA overrun, ");
			if (amt < 0)
				return (amt);
			p += amt;
			maxcnt -= amt;
		}
		amt = js_snprintf(p, maxcnt, "resid: %d\n", scgp->scmd->resid);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	if (scgp->verbose > 0 || (ret < 0 && scgp->silent <= 0)) {
		amt = scg_sprintresult(scgp, p, maxcnt);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	return (p - buf);
}

/*
 * Set up SCSI error buffer with verbose print data.
 * Action depends on SCSI verbose status.
 */
void
scg_vsetup(SCSI *scgp)
{
	scg_vhead(scgp);
	scg_vtail(scgp);
}

/*
 * Return the residual DMA count for last command.
 * If this count is < 0, then a DMA overrun occured.
 */
int
scg_getresid(SCSI *scgp)
{
	return (scgp->scmd->resid);
}

/*
 * Return the actual DMA count for last command.
 */
int
scg_getdmacnt(SCSI *scgp)
{
	register struct scg_cmd *scmd = scgp->scmd;

	if (scmd->resid < 0)
		return (scmd->size);

	return (scmd->size - scmd->resid);
}

/*
 * Test if last SCSI command got an error.
 */
BOOL
scg_cmd_err(SCSI *scgp)
{
	register struct scg_cmd *cp = scgp->scmd;

	if (cp->error != SCG_NO_ERROR ||
				cp->ux_errno != 0 ||
				*(Uchar *)&cp->scb != 0 ||
				cp->u_sense.cmd_sense[0] != 0)	/* Paranioa */
		return (TRUE);
	return (FALSE);
}

/*
 * Used to print error messges if the command itself has been run silently.
 *
 * print the following SCSI codes:
 *
 * -	command transport status
 * -	CDB
 * -	SCSI status byte
 * -	Sense Bytes
 * -	Decoded Sense data
 * -	DMA status
 * -	SCSI timing
 *
 * to SCSI errfile.
 */
void
scg_printerr(SCSI *scgp)
{
	scg_fprinterr(scgp, (FILE *)scgp->errfile);
}

/*
 * print the following SCSI codes:
 *
 * -	command transport status
 * -	CDB
 * -	SCSI status byte
 * -	Sense Bytes
 * -	Decoded Sense data
 * -	DMA status
 * -	SCSI timing
 *
 * to a file.
 */
void
scg_fprinterr(SCSI *scgp, FILE *f)
{
	char	errbuf[SCSI_ERRSTR_SIZE];
	int	amt;

	amt = scg_sprinterr(scgp, errbuf, sizeof (errbuf));
	if (amt > 0) {
		filewrite(f, errbuf, amt);
		fflush(f);
	}
}

/*
 * print the following SCSI codes:
 *
 * -	command transport status
 * -	CDB
 * -	SCSI status byte
 * -	Sense Bytes
 * -	Decoded Sense data
 * -	DMA status
 * -	SCSI timing
 *
 * into a buffer.
 */
int
scg_sprinterr(SCSI *scgp, char *buf, int maxcnt)
{
	int	amt;
	int	osilent = scgp->silent;
	int	overbose = scgp->verbose;

	scgp->silent = 0;
	scgp->verbose = 0;
	amt = scg_svtail(scgp, NULL, buf, maxcnt);
	scgp->silent = osilent;
	scgp->verbose = overbose;
	return (amt);
}

/*
 * print the following SCSI codes:
 *
 * -	command transport status
 * -	CDB
 * -	SCSI status byte
 * -	Sense Bytes
 * -	Decoded Sense data
 *
 * into a buffer.
 */
int
scg__sprinterr(SCSI *scgp, char *buf, int maxcnt)
{
	register struct scg_cmd *cp = scgp->scmd;
	register char		*err;
		char		*cmdname = "SCSI command name not set by caller";
		char		errbuf[64];
	register char		*p = buf;
	register int		amt;

	switch (cp->error) {

	case SCG_NO_ERROR :	err = "no error"; break;
	case SCG_RETRYABLE:	err = "retryable error"; break;
	case SCG_FATAL    :	err = "fatal error"; break;
				/*
				 * We need to cast timeval->* to long because
				 * of the broken sys/time.h in Linux.
				 */
	case SCG_TIMEOUT  :	js_snprintf(errbuf, sizeof (errbuf),
					"cmd timeout after %ld.%03ld (%d) s",
					(long)scgp->cmdstop->tv_sec,
					(long)scgp->cmdstop->tv_usec/1000,
								cp->timeout);
				err = errbuf;
				break;
	default:		js_snprintf(errbuf, sizeof (errbuf),
					"error: %d", cp->error);
				err = errbuf;
	}

	if (scgp->cmdname != NULL && scgp->cmdname[0] != '\0')
		cmdname = scgp->cmdname;
	amt = serrmsgno(cp->ux_errno, p, maxcnt, "%s: scsi sendcmd: %s\n", cmdname, err);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	amt = scg_sprintcdb(scgp, p, maxcnt);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	if (cp->error <= SCG_RETRYABLE) {
		amt = scg_sprintstatus(scgp, p, maxcnt);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}

	if (cp->scb.chk) {
		amt = scg_sprsense(p, maxcnt, (Uchar *)&cp->sense, cp->sense_count);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
		amt = scg__errmsg(scgp, p, maxcnt, &cp->sense, &cp->scb, -1);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	return (p - buf);
}

/*
 * XXX Do we need this function?
 *
 * print the SCSI Command descriptor block to XXX stderr.
 */
void
scg_printcdb(SCSI *scgp)
{
	scg_prbytes("CDB: ", scgp->scmd->cdb.cmd_cdb, scgp->scmd->cdb_len);
}

/*
 * print the SCSI Command descriptor block into a buffer.
 */
int
scg_sprintcdb(SCSI *scgp, char *buf, int maxcnt)
{
	int	cnt;

	cnt = scg_sprbytes(buf, maxcnt, "CDB: ", scgp->scmd->cdb.cmd_cdb, scgp->scmd->cdb_len);
	if (cnt < 0)
		cnt = 0;
	return (cnt);
}

/*
 * XXX Do we need this function?
 * XXX scg_printrdata() is used.
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print the SCSI send data to stderr.
 */
void
scg_printwdata(SCSI *scgp)
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) == 0) {
		js_fprintf(stderr, "Sending %d (0x%X) bytes of data.\n",
			scmd->size, scmd->size);
		scg_prbytes("Write Data: ",
			(Uchar *)scmd->addr,
			scmd->size > 100 ? 100 : scmd->size);
	}
}

/*
 * print the SCSI send data into a buffer.
 */
int
scg_sprintwdata(SCSI *scgp, char *buf, int maxcnt)
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	register char		*p = buf;
	register int		amt;

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) == 0) {
		amt = js_snprintf(p, maxcnt,
			"Sending %d (0x%X) bytes of data.\n",
			scmd->size, scmd->size);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
		amt = scg_sprbytes(p, maxcnt, "Write Data: ",
			(Uchar *)scmd->addr,
			scmd->size > 100 ? 100 : scmd->size);
		if (amt < 0)
			return (amt);
		p += amt;
	}
	return (p - buf);
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print the SCSI received data to stderr.
 */
void
scg_printrdata(SCSI *scgp)
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	register int		trcnt = scg_getdmacnt(scgp);

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) != 0) {
		js_fprintf(stderr, "Got %d (0x%X), expecting %d (0x%X) bytes of data.\n",
			trcnt, trcnt,
			scmd->size, scmd->size);
		scg_prbytes("Received Data: ",
			(Uchar *)scmd->addr,
			trcnt > 100 ? 100 : trcnt);
	}
}

/*
 * print the SCSI received data into a buffer.
 */
int
scg_sprintrdata(SCSI *scgp, char *buf, int maxcnt)
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	register char		*p = buf;
	register int		amt;
	register int		trcnt = scg_getdmacnt(scgp);

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) != 0) {
		amt = js_snprintf(p, maxcnt,
			"Got %d (0x%X), expecting %d (0x%X) bytes of data.\n",
			trcnt, trcnt,
			scmd->size, scmd->size);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
		amt = scg_sprbytes(p, maxcnt,
			"Received Data: ",
			(Uchar *)scmd->addr,
			trcnt > 100 ? 100 : trcnt);
		if (amt < 0)
			return (amt);
		p += amt;
	}
	return (p - buf);
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print the SCSI timings and (depending on verbose) received data to stderr.
 */
void
scg_printresult(SCSI *scgp)
{
	js_fprintf(stderr, "cmd finished after %ld.%03lds timeout %ds\n",
		(long)scgp->cmdstop->tv_sec,
		(long)scgp->cmdstop->tv_usec/1000,
		scgp->scmd->timeout);
	if (scgp->verbose > 1)
		scg_printrdata(scgp);
	flush();
}

/*
 * print the SCSI timings and (depending on verbose) received data into a buffer.
 */
int
scg_sprintresult(SCSI *scgp, char *buf, int maxcnt)
{
	register char		*p = buf;
	register int		amt;

	amt = js_snprintf(p, maxcnt,
		"cmd finished after %ld.%03lds timeout %ds\n",
		(long)scgp->cmdstop->tv_sec,
		(long)scgp->cmdstop->tv_usec/1000,
		scgp->scmd->timeout);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;
	if (scgp->verbose > 1) {
		amt = scg_sprintrdata(scgp, p, maxcnt);
		if (amt < 0)
			return (amt);
		p += amt;
	}
	return (p - buf);
}

/*
 * XXX Do we need this function?
 *
 * print the SCSI status byte in human readable form to the SCSI error file.
 */
void
scg_printstatus(SCSI *scgp)
{
	char	errbuf[SCSI_ERRSTR_SIZE];
	int	amt;

	amt = scg_sprintstatus(scgp, errbuf, sizeof (errbuf));
	if (amt > 0) {
		filewrite((FILE *)scgp->errfile, errbuf, amt);
		fflush((FILE *)scgp->errfile);
	}
}

/*
 * print the SCSI status byte in human readable form into a buffer.
 */
int
scg_sprintstatus(SCSI *scgp, char *buf, int maxcnt)
{
	register struct scg_cmd *cp = scgp->scmd;
		char	*err;
		char	*err2 = "";
	register char	*p = buf;
	register int	amt;

	amt = js_snprintf(p, maxcnt, "status: 0x%x ", *(Uchar *)&cp->scb);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;
#ifdef	SCSI_EXTENDED_STATUS
	if (cp->scb.ext_st1) {
		amt = js_snprintf(p, maxcnt, "0x%x ", ((Uchar *)&cp->scb)[1]);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	if (cp->scb.ext_st2) {
		amt = js_snprintf(p, maxcnt, "0x%x ", ((Uchar *)&cp->scb)[2]);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
#endif
	switch (*(Uchar *)&cp->scb & 036) {

	case 0  : err = "GOOD STATUS";			break;
	case 02 : err = "CHECK CONDITION";		break;
	case 04 : err = "CONDITION MET/GOOD";		break;
	case 010: err = "BUSY";				break;
	case 020: err = "INTERMEDIATE GOOD STATUS";	break;
	case 024: err = "INTERMEDIATE CONDITION MET/GOOD"; break;
	case 030: err = "RESERVATION CONFLICT";		break;
	default : err = "Reserved";			break;
	}
#ifdef	SCSI_EXTENDED_STATUS
	if (cp->scb.ext_st1 && cp->scb.ha_er)
		err2 = " host adapter detected error";
#endif
	amt = js_snprintf(p, maxcnt, "(%s%s)\n", err, err2);
	if (amt < 0)
		return (amt);
	p += amt;
	return (p - buf);
}

/*
 * print some bytes in hex to a file.
 */
void
scg_fprbytes(FILE *f, char *s, register Uchar *cp, register int n)
{
	js_fprintf(f, "%s", s);
	while (--n >= 0)
		js_fprintf(f, " %02X", *cp++);
	js_fprintf(f, "\n");
}

/*
 * print some bytes in ascii to a file.
 */
void
scg_fprascii(FILE *f, char *s, register Uchar *cp, register int n)
{
	register int	c;

	js_fprintf(f, "%s", s);
	while (--n >= 0) {
		c = *cp++;
		if (c >= ' ' && c < 0177)
			js_fprintf(f, "%c", c);
		else
			js_fprintf(f, ".");
	}
	js_fprintf(f, "\n");
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print some bytes in hex to stderr.
 */
void
scg_prbytes(char *s, register Uchar *cp, register int n)
{
	scg_fprbytes(stderr, s, cp, n);
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print some bytes in ascii to stderr.
 */
void
scg_prascii(char *s, register Uchar *cp, register int n)
{
	scg_fprascii(stderr, s, cp, n);
}

/*
 * print some bytes in hex into a buffer.
 */
int
scg_sprbytes(char *buf, int maxcnt, char *s, register Uchar *cp, register int n)
{
	register char	*p = buf;
	register int	amt;

	amt = js_snprintf(p, maxcnt, "%s", s);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	while (--n >= 0) {
		amt = js_snprintf(p, maxcnt, " %02X", *cp++);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	amt = js_snprintf(p, maxcnt, "\n");
	if (amt < 0)
		return (amt);
	p += amt;
	return (p - buf);
}

/*
 * print some bytes in ascii into a buffer.
 */
int
scg_sprascii(char *buf, int maxcnt, char *s, register Uchar *cp, register int n)
{
	register char	*p = buf;
	register int	amt;
	register int	c;

	amt = js_snprintf(p, maxcnt, "%s", s);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	while (--n >= 0) {
		c = *cp++;
		if (c >= ' ' && c < 0177)
			amt = js_snprintf(p, maxcnt, "%c", c);
		else
			amt = js_snprintf(p, maxcnt, ".");
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	amt = js_snprintf(p, maxcnt, "\n");
	if (amt < 0)
		return (amt);
	p += amt;
	return (p - buf);
}

/*
 * print the SCSI sense data for last command in hex to a file.
 */
void
scg_fprsense(FILE *f, Uchar *cp, int n)
{
	scg_fprbytes(f, "Sense Bytes:", cp, n);
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print the SCSI sense data for last command in hex to stderr.
 */
void
scg_prsense(Uchar *cp, int n)
{
	scg_fprsense(stderr, cp, n);
}

/*
 * print the SCSI sense data for last command in hex into a buffer.
 */
int
scg_sprsense(char *buf, int maxcnt, Uchar *cp, int n)
{
	return (scg_sprbytes(buf, maxcnt, "Sense Bytes:", cp, n));
}

/*
 * Return the SCSI status byte for last command.
 */
int
scg_cmd_status(SCSI *scgp)
{
	struct scg_cmd	*cp = scgp->scmd;
	int		cmdstatus = *(Uchar *)&cp->scb;

	return (cmdstatus);
}

/*
 * Return the SCSI sense key for last command.
 */
int
scg_sense_key(SCSI *scgp)
{
	register struct scg_cmd *cp = scgp->scmd;
	int	key = -1;

	if (!scg_cmd_err(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
		key = ((struct scsi_ext_sense *)&(cp->sense))->key;
	return (key);
}

/*
 * Return all the SCSI sense table last command.
 */
unsigned char *
scg_sense_table(SCSI *scgp)
{
	register struct scg_cmd *cp = scgp->scmd;

	if(!scg_cmd_err(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
	return &(cp->sense);
}


/*
 * Return the SCSI sense code for last command.
 */
int
scg_sense_code(SCSI *scgp)
{
	register struct scg_cmd *cp = scgp->scmd;
	int	code = -1;

	if (!scg_cmd_err(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
		code = ((struct scsi_ext_sense *)&(cp->sense))->sense_code;
	else
		code = cp->sense.code;
	return (code);
}

/*
 * Return the SCSI sense qualifier for last command.
 */
int
scg_sense_qual(SCSI *scgp)
{
	register struct scg_cmd *cp = scgp->scmd;

	if (!scg_cmd_err(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
		return (((struct scsi_ext_sense *)&(cp->sense))->qual_code);
	else
		return (0);
}

/*
 * Print the device type from the SCSI inquiry buffer to file.
 */
void
scg_fprintdev(FILE *f, struct scsi_inquiry *ip)
{
	if (ip->removable)
		js_fprintf(f, "Removable ");
	if (ip->data_format >= 2) {
		switch (ip->qualifier) {

		case INQ_DEV_PRESENT:
			break;
		case INQ_DEV_NOTPR:
			js_fprintf(f, "not present ");
			break;
		case INQ_DEV_RES:
			js_fprintf(f, "reserved ");
			break;
		case INQ_DEV_NOTSUP:
			if (ip->type == INQ_NODEV) {
				js_fprintf(f, "unsupported\n"); return;
			}
			js_fprintf(f, "unsupported ");
			break;
		default:
			js_fprintf(f, "vendor specific %d ",
						(int)ip->qualifier);
		}
	}
	switch (ip->type) {

	case INQ_DASD:
		js_fprintf(f, "Disk");		break;
	case INQ_SEQD:
		js_fprintf(f, "Tape");		break;
	case INQ_PRTD:
		js_fprintf(f, "Printer");	break;
	case INQ_PROCD:
		js_fprintf(f, "Processor");	break;
	case INQ_WORM:
		js_fprintf(f, "WORM");		break;
	case INQ_ROMD:
		js_fprintf(f, "CD-ROM");	break;
	case INQ_SCAN:
		js_fprintf(f, "Scanner");	break;
	case INQ_OMEM:
		js_fprintf(f, "Optical Storage"); break;
	case INQ_JUKE:
		js_fprintf(f, "Juke Box");	break;
	case INQ_COMM:
		js_fprintf(f, "Communication");	break;
	case INQ_IT8_1:
		js_fprintf(f, "IT8 1");		break;
	case INQ_IT8_2:
		js_fprintf(f, "IT8 2");		break;
	case INQ_STARR:
		js_fprintf(f, "Storage array");	break;
	case INQ_ENCL:
		js_fprintf(f, "Enclosure services"); break;
	case INQ_SDAD:
		js_fprintf(f, "Simple direct access"); break;
	case INQ_OCRW:
		js_fprintf(f, "Optical card r/w"); break;
	case INQ_BRIDGE:
		js_fprintf(f, "Bridging expander"); break;
	case INQ_OSD:
		js_fprintf(f, "Object based storage"); break;
	case INQ_ADC:
		js_fprintf(f, "Automation/Drive Interface"); break;
	case INQ_WELLKNOWN:
		js_fprintf(f, "Well known lun"); break;

	case INQ_NODEV:
		if (ip->data_format >= 2) {
			js_fprintf(f, "unknown/no device");
			break;
		} else if (ip->qualifier == INQ_DEV_NOTSUP) {
			js_fprintf(f, "unit not present");
			break;
		}
	default:
		js_fprintf(f, "unknown device type 0x%x",
						(int)ip->type);
	}
	js_fprintf(f, "\n");
}

/*
 * Print the device type from the SCSI inquiry buffer to stdout.
 */
void
scg_printdev(struct scsi_inquiry *ip)
{
	scg_fprintdev(stdout, ip);
}

#include <vadefs.h>

/*
 * print into the SCSI error buffer, adjust the next write pointer.
 */
/* VARARGS2 */
int
scg_printf(SCSI *scgp, const char *form, ...)
{
	int	cnt;
	va_list	args;

	va_start(args, form);
	cnt = js_snprintf(scgp->errptr, scg_errrsize(scgp), "%r", form, args);
	va_end(args);

	if (cnt < 0) {
		scgp->errptr[0] = '\0';
	} else {
		scgp->errptr += cnt;
	}
	return (cnt);
}

/*
 * Flush the SCSI error buffer to SCSI errfile.
 * Clear error buffer after flushing.
 */
int
scg_errflush(SCSI *scgp)
{
	if (scgp->errfile == NULL)
		return (0);

	return (scg_errfflush(scgp, (FILE *)scgp->errfile));
}

/*
 * Flush the SCSI error buffer to a file.
 * Clear error buffer after flushing.
 */
int
scg_errfflush(SCSI *scgp, FILE *f)
{
	int	cnt;

	cnt = scgp->errptr - scgp->errbeg;
	if (cnt > 0) {
		filewrite(f, scgp->errbeg, cnt);
		fflush(f);
		scgp->errbeg = scgp->errptr;
	}
	return (cnt);
}
