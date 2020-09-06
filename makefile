#
# Guide to Variables
#
# File groupings - these groupings partition all the component files into
# buckets for purposes of source code control.  Such variables consist
# of a prefix and a suffix.
#
# The prefixes are:
#
#	C - C file
# H - C include file
# R - Rez file
# Rsrc - Resource file
#
# The suffixes are:
#
#	Source - this is a source file, one to be managed and not thrown away
# Deriv - this file is generated from some other file and can be remade at will
#
#
# product names
#
Debug = eudora51-PPC-debug
Carbon = eudora51-carbon
DebugCarbon = eudora51-carbon-debug
Help	  = "Eudora Help"

#
# Options for various processors
#
RezOptions	= -i :include:
SendAE		= mwsend -timeout 72000


#
# Source files we generate
#
CDerivs = �
	prefaudit.c �
	preflimits.c �
	prefenable.c �
	filtdefs.c �
	unloadseg.c �
	auditdefs.c �

#
# Sources for version 2.1
#
CSources = �
	acap.c	�
	address.c	�
	aeutil.c	�
	anal.c	�
	appcdef.c	�
	appleevent.c	�
	binhex.c	�
	boxact.c	�
	buildtoc.c	�
	color.c		�
	comp.c		�
	compact.c	�
	concentrator.c	�
	dflutils.c	�
	emoticon.c	�
	export.c	�
	fmtbar.c	�
	ctb.c		�
	cursor.c	�
	ends.c		�
	euErrors.c		�
	fileutil.c	�
	filters.c	�
	filtwin.c	�
	filtmng.c	�
	filtrun.c	�
	filtthread.c	�
	filegraphic.c		�
	find.c		�
	functions.c	�
	globals.c	�
	header.c	�
	hexbin.c	�
	icon.c		�
	inet.c		�
	junk.c		�
	ldaplibglue.c	�
	ldaputils.c		�
	ldef.c		�
	lex822.c	�
	light.c	�
	lineio.c	�
	link.c		�
	listcdef.c		�
	lmgr.c		�
	log.c		�
	mailbox.c	�
	toc.c	�
	mailxfer.c	�
	main.c		�
	mbdrawer.c	�
	mbwin.c		�
	md5.c		�
	menu.c		�
	menusharing.c		�
	messact.c	�
	message.c	�
	mime.c		�
	modeless.c	�
	multi.c	�
	mywindow.c	�
	nickexp.c	�
	nickmng.c	�
	nickwin.c	�
	nickae.c	�
	register.c	�
	pgpin.c		�
	ph.c		�
	pop.c		�
	prefs.c		�
	print.c		�
	progress.c	�
	proxy.c	�
	peteglue.c	�
	regexp.c	�
	rich.c	�
	html.c	�
	sasl.c	�
	search.c	�
	searchwin.c	�
	schizo.c	�
	sendmail.c	�
	setldef.c	�
	tlate_ldef.c	�
	shame.c		�
	sort.c		�
	speechutil.c		�
	spell.c		�
	squish.c	�
	stickypopup.c		�
	stringutil.c		�
	toolbar.c		�
	toolbarpopup.c		�
	trans.c		�
	tcp.c		�
	tefuncs.c	�
	textw.c		�
	timebomb.c		�
	oops.c		�
	url.c		�
	util.c		�
	utl.c		�
	uudecode.c	�
	uupc.c		�
	winutil.c	�
	personalitieswin.c	�
	threading.c	�
	task_ldef.c	�
	taskProgress.c	�
	signaturewin.c	�
	stationerywin.c	�
	floatingwin.c	�
	listview.c	�
	wazoo.c	�
	mstore.c	�
	msmaildb.c	�
	msiddb.c	�
	mstoc.c	�
	msinfo.c	�
	filt_ldef.c	�
	appear_util.c	�
	TransStream.c	�
	makefilter.c	�
	table.c	�
	imapnetlib.c	�
	imapmailboxes.c	�
	downloadurl.c	�
	spool.c	�
	dial.c	�
	adwin.c	�
	linkwin.c	�
	linkmng.c	�
	graph.c	�
	statmng.c	�
	statwin.c	�
	import.c	�
	xml.c	�
	scriptmenu.c	�
	carbonutil.c	�
	fileview.c	�
	palmconduitae.c	�
	osxabsync.cp	�
	
#
# include files
#
HSources = �
	:include:acap.h	�
	:include:address.h	�
	:include:aeutil.h	�
	:include:allheaders.h	�
	:include:anal.h	�
	:include:Appearance.h	�
	:include:appleevent.h	�
	:include:binhex.h	�
	:include:boxact.h	�
	:include:buildtoc.h	�
	:include:cboxact.h	�
	:include:CodeFragmentsSupplement.h	�
	:include:color.h	�
	:include:comp.h		�
	:include:compact.h	�
	:include:concentrator.h	�
	:include:fmtbar.h	�
	:include:conf.h		�
	:include:ctb.h		�
	:include:cursor.h	�
	:include:dflsuppl.h	�
	:include:dflutils.h	�
	:include:emoticon.h	�
	:include:euErrors.h		�
	:include:export.h		�
	:include:ends.h		�
	:include:filegraphic.h	�
	:include:fileutil.h	�
	:include:filters.h	�
	:include:filtmng.h	�
	:include:filtwin.h	�
	:include:filtrun.h	�
	:include:filtthread.h	�
	:include:find.h		�
	:include:functions.h	�
	:include:header.h	�
	:include:hexbin.h	�
	:include:icon.h		�
	:include:inet.h		�
	:include:junk.h		�
	:include:KrbDriver.h	�
	:include:lber.h	�
	:include:ldap.h	�
	:include:ldaplibglue.h	�
	:include:ldaputils.h	�
	:include:lex822.h	�
	:include:light.h	�
	:include:lineio.h	�
	:include:link.h		�
	:include:listcdef.h		�
	:include:lmgr.h		�
	:include:log.h		�
	:include:mailbox.h	�
	:include:toc.h	�
	:include:mailxfer.h	�
	:include:main.h		�
	:include:mbdrawer.h	�
	:include:mbwin.h	�
	:include:md5.h		�
	:include:menu.h		�
	:include:menusharing.h		�
	:include:messact.h	�
	:include:message.h	�
	:include:mime.h		�
	:include:modeless.h	�
	:include:multi.h	�
	:include:MyRes.h	�
	:include:mywindow.h	�
	:include:nickexp.h	�
	:include:nickmng.h	�
	:include:nickwin.h	�
	:include:nickae.h	�
	:include:appcdef.h	�
	:include:register.h	�
	:include:numcode.h	�
	:include:passwd.h	�
	:include:pgpin.h		�
	:include:pgpout.h		�
	:include:ph.h		�
	:include:pop.h		�
	:include:prefs.h	�
	:include:print.h	�
	:include:progress.h	�
	:include:proxy.h	�
	:include:peteglue.h	�
	:include:regexp.h	�
	:include:rich.h	�
	:include:html.h	�
	:include:sasl.h	�
	:include:search.h	�
	:include:searchwin.h	�
	:include:speechutil.h	�
	:include:schizo.h	�
	:include:sendmail.h	�
	:include:shame.h	�
	:include:sort.h		�
	:include:spell.h		�
	:include:squish.h	�
	:include:stickypopup.h	�
	:include:StringUtil.h �
	:include:task_ldef.h		�
	:include:tcp.h		�
	:include:tefuncs.h	�
	:include:text.h		�
	:include:timebomb.h		�
	:include:toolbar.h		�
	:include:toolbarpopup.h		�
	:include:trans.h		�
	:include:oops.h		�
	:include:url.h		�
	:include:util.h		�
	:include:utl.h		�
	:include:uudecode.h	�
	:include:uupc.h		�
	:include:winutil.h	�
	":Editor:Source:Application Headers:pete.h"	�
	:include:macslip.h	�
	:include:personalitieswin.h	�
	:include:threading.h	�
	:include:taskProgress.h	�
	:include:signaturewin.h	�
	:include:stationerywin.h	�
	:include:floatingwin.h	�
	:include:listview.h	�
	:include:wazoo.h	�
	:include:mstore.h	�
	:include:msmaildb.h	�
	:include:msiddb.h	�
	:include:mstoc.h	�
	:include:msinfo.h	�
	:include:appear_util.h	�
	:include:MyDefs.h	�
	:include:TransStream.h	�
	:include:makefilter.h	�
	:include:table.h	�
	:include:imapmailboxes.h	�
	:include:imapnetlib.h	�
	:include:downloadurl.h	�
	:include:spool.h	�
	:include:dial.h	�
	:include:adwin.h	�
	:include:audit.h	�
	:include:linkwin.h	�
	:include:linkmng.h	�
	:include:graph.h	�
	:include:statmng.h	�
	:include:statwin.h	�
	:include:import.h	�
	:include:xml.h	�
	:include:scriptmenu.h	�
	:include:carbonutil.h	�
	:include:fileview.h	�
	:include:palmconduitae.h	�
	:include:osxabsync.h	�
	
#
# Text resource files
#
TextFiles = �
	:TEXT:Intro=1002 �
	:TEXT:RegNag=1003 �
	:TEXT:LightDowngrade1=1004 �
	:TEXT:AskAudit=1005 �
	:TEXT:LighDowngrade2=1006 �
	:TEXT:FullFeature1=1007 �
	:TEXT:FullFeature2=1008 �
	:TEXT:AdTrouble1=1009 �
	:TEXT:Deadbeat=1010 �
	:TEXT:AdTrouble2=1011 �
	:TEXT:AdTheObscure=1012 �
	:TEXT:AuditLegend=1013 �
	:TEXT:PrePayment=1014 �
	:TEXT:PreRegistration=1015 �
	:TEXT:PreProfiling=1016 �
	:TEXT:PleaseProfile2=1017 �
	:TEXT:Repay=1018 �
	:TEXT:JunkIntro=1019 �
	:TEXT:ConcentratorProfiles=1020.xml �
	:TEXT:JunkDowngrade=1021 �
	:TEXT:Suggest=1022 �
	:TEXT:Bug=1023 �

#
# Help Text resource files
#
HelpTextFiles = �
	':HelpTEXT:=2001'�
	':HelpTEXT:=2007'�
	':HelpTEXT:=2013'�
	':HelpTEXT:=2016'�
	':HelpTEXT:=2020'�
	':HelpTEXT:About this Release=2000'�
	':HelpTEXT:Attaching a File=2003'�
	':HelpTEXT:Changing Your Password=2019'�
	':HelpTEXT:Creating Messages=2002'�
	':HelpTEXT:Deleting Messages=2010'�
	':HelpTEXT:Filtering Messages=2012'�
	':HelpTEXT:LightHelp=3001'�
	':HelpTEXT:Modifiers and Shortcuts=2017'�
	':HelpTEXT:Questions or problems?=2021'�
	':HelpTEXT:Receiving Mail=2008'�
	':HelpTEXT:Replying to Messages=2009'�
	':HelpTEXT:SupportHelp=3000'�
	':HelpTEXT:Transferring Messages=2011'�
	':HelpTEXT:Using Mailboxes=2014'�
	':HelpTEXT:Using Personalities=2006'�
	':HelpTEXT:Using Signatures=2004'�
	':HelpTEXT:Using Stationery=2005'�
	':HelpTEXT:Using the Address Book=2015'�

#
# include files we generate
#
HDerivs	= �
	 :include:Globals.h	�
	 :include:StringDefs.h	�
	 :include:StrnDefs.h	�
	 :include:prefdefs.h	�
	 :include:filtdefs.h	�
	 :include:auditdefs.h	�

HFiles = {HSources} {HDerivs}

#
# Source .r files
#
RSources = �
	SMTP.r			�
	mappings.r		�
	version.r		�
	aete.r			�
	Two.r				�

#
# Definition files
#
DefSources = �
	StringDefs �
	StrnDefs �
	PrefDefs �

#
# Common Rez inputs
#
RCommon = �
	SMTP.r			�
	mappings.r		�
	aete.r			�
	:AddressBookTabs:homeTab.r �
	:AddressBookTabs:notesTab.r �
	:AddressBookTabs:otherTab.r �
	:AddressBookTabs:personalTab.r �
	:AddressBookTabs:photoTab.r �
	:AddressBookTabs:workTab.r �

#
# Rez inputs for the commercial version
#
RTwo = �
	Two.r				�
	
#
# Rez input we generate
#
RDerivs	= �
	StringDefs.strn		�
	StrnDefs.strn		�
	HelpMenuDefs.hmnu	�
	HelpDlogDefs.strn	�
	HelpDlogDefs.hdlg	�
	PrefDefs.strn		�
	StringDefsH.strn	�
	FiltDefs.r	�
	acap.r	�
	audit.r �
	Text.r �

#
# resource files we generate
#
RsrcDerivs = �
	Light.rsrc �
	SettingsTwo.rsrc �
	Two.rsrc �
	Common.rsrc �
	30.rsrc �
	Icons.rsrc �
	SettingsIcons.rsrc �
	ToolbarIcons.rsrc �
	Esoteric Settings 6.0 �


Derivs = {RDerivs} {CDerivs} {HDerivs} {RsrcDerivs} EudoraNotify.osax {Help} 

#
# HelpSrc - where the help comes from
#
HelpSrc = �
	Help.rsrc	�
	ToolbarIcons.rsrc	�
	Help.r	�
	Nick.r		�
	MacTcpErrors.r	�
	HelpMenuDefs.hmnu	�
	HelpDlogDefs.hdlg	�
	HelpDlogDefs.strn	�
	PrefDefs.strn	�
	StringDefsH.strn	�

#
# pre-built rsrc files
#
RsrcTwo = �
	credits.rsrc		�
	Help.rsrc		�
	ldaputils.rsrc		�
	RegTwo.rsrc		�
	SettingsIcons.rsrc �
	ShLibDirAlias.rsrc		�
	Icons.rsrc �
	nagWacky.rsrc �

RsrcSources = �
	Help.rsrc		�
	ldaputils.rsrc		�
	ShLibDirAlias.rsrc		�
	RegTwo.rsrc		�
	common.rsrc		�
	credits.rsrc		�
	LightSrc.rsrc		�
	LightSettings.rsrc	�
	Netcom.rsrc	�
	nagWacky.rsrc �

ScriptSources = �
	:bits:buildprefs �
	:bits:makeindex �
	:bits:makesegs �
	:bits:ProcessDlogHelp �
	:bits:processFilt �
	:bits:ProcessMenuHelp �
	:bits:ProcessStrings �
	:bits:ProcessStrRes �
	:bits:AddDlgx �
	:bits:buildCredits �

Sources = {HSources} {CSources} {RSources} {DefSources} {ScriptSources} makefile

{Derivs} � :include:conf.h	# and depends on conf.h

###################################################
###################################################
# build the program
###################################################
###################################################

#
# default targets
#
it	� {Carbon}
db	� {DebugCarbon}
carbon	� {Carbon}
dbcarbon	� {DebugCarbon}
help � {Help} 'Esoteric Settings 6.0'

{Carbon} � Parts
	cw Eudora.proj
	{sendae} -t "{CodeWarrior}" -e MMPRSTrg -----TEXT "Carbon"
	{sendae} -t "{CodeWarrior}" -e MMPRMake

{DebugCarbon} � Parts
	cw Eudora.proj
	{sendae} -t "{CodeWarrior}" -e MMPRSTrg -----TEXT "Debug Carbon"
	{sendae} -t "{CodeWarrior}" -e MMPRMake

TwoPre � Parts {Help}
	
#
# build the help
#
{Help} � {HelpSrc} HelpText.r
	cat �.xset >X-Eudora-Settings.txt
	Rez {RezOptions} -d TWO -t EuHl -c CSOm Help.r -o {Help}

'Esoteric Settings 6.0' � EsotericSettings60.r
	rz -t rsrc -c CSOm -o 'Esoteric Settings 6.0' EsotericSettings60.r 

#
# build resource files
#
Parts � 30.rsrc Two.dlgx.rsrc {CSources} TwoName {RsrcTwo} {HDerivs} unloadseg.c common.rsrc two.rsrc SettingsTwo.rsrc Eudora.rsrc.plist
30.rsrc � Common.r {RTwo} TwoShell.r audit.r Text.r Credits.r �
		:include:StringDefs.h :include:Strndefs.h :include:PrefDefs.h :include:FiltDefs.h �
		{RCommon} :include:buildversion.h SettingsIcons.rsrc Icons.rsrc acap.r
	Rez {RezOptions} -t rsrc -c RSED TwoShell.r -o 30.rsrc
	OrphanFiles 30.rsrc

Two.dlgx.rsrc � 30.rsrc {RsrcTwo} SettingsTwo.rsrc common.rsrc
	derez 30.rsrc -only DLOG -only ALRT Types.r > temp.DLOG.r
	for r in {RsrcTwo} SettingsTwo.rsrc common.rsrc
		derez "{r}" -only DLOG -only ALRT Types.r >> temp.DLOG.r
	end
	perl :bits:AddDlgx temp.DLOG.r > temp.dlgx.r
	rez -t rsrc -c RSED Dialogs.r temp.dlgx.r -o Two.dlgx.rsrc

Eudora.rsrc.plist � Eudora.plist TwoName
	perl -p -e 's/EUDORA_VERSION/'`cat TwoName`'/;' Eudora.plist >Eudora.rsrc.plist

common.rsrc � common.rsrc.r
	rz -o common.rsrc common.rsrc.r

two.rsrc � two.rsrc.r
	rz -o two.rsrc two.rsrc.r

SettingsTwo.rsrc � SettingsTwo.rsrc.r
	rz -o SettingsTwo.rsrc SettingsTwo.rsrc.r

EudoraNotify.osax � osax.r
	Rez {RezOptions} -t 'osax' -c 'ascr' osax.r -o EudoraNotify.osax

#
# preprocessing of various sorts
#
stringdefs.acap :include:StringDefs.h StringDefs.strn StringDefsH.strn � StringDefs :bits:processstrings
	:Bits:ProcessStrings StringDefs

:include:Globals.h � Globals.c
	perl -e '<>;<>;while(<>){s/^[A-Za-z]+/extern $&/;s/ = .*/;/;s/^\s*{.*\n//;print;}' Globals.c >:include:Globals.h
	
unloadseg.c � :bits:makesegs
	perl :Bits:makesegs UnloadSeg.c :include:numcode.h {CSources}

prefs.acap PrefDefs.strn :include:PrefDefs.h preflimits.c � :bits:buildprefs PrefDefs PrefDefs.extras
	perl :bits:buildprefs PrefDefs

ToolbarIcons.rsrc �
	makeicons -y 29999 ToolbarIcons ToolbarIcons.rsrc
SettingsIcons.rsrc �
	makeicons -y 30499 SettingsIcons SettingsIcons.rsrc
Icons.rsrc �
	makeicons -y 24999 Icons Icons.rsrc

HelpDlogDefs.hdlg HelpDlogDefs.strn � HelpDlogDefs :bits:processdloghelp HelpDlogDefs.extras
	:Bits:ProcessDlogHelp HelpDlogDefs 17100

HelpMenuDefs.hmnu � HelpMenuDefs HelpMenuDefs.extras :bits:processmenuhelp
	:Bits:ProcessMenuHelp HelpMenuDefs 14000

:include:StrnDefs.h StrnDefs.strn StrnDefs.doc � StrnDefs :bits:processStrings
	perl :bits:processStrRes StrnDefs

:include:FiltDefs.h FiltDefs.c FiltDefs.r � FiltDefs FiltDefs.extras :bits:processFilt
	perl :bits:processFilt FiltDefs

:include:auditdefs.h auditdefs.c audit.r � auditdefs :bits:buildaudit
	perl :bits:buildaudit auditdefs

Text.r � {TextFiles} :bits:buildtext
	perl :bits:buildtext {TextFiles} >Text.r

HelpText.r � {HelpTextFiles} :bits:buildtext
	perl :bits:buildtext {HelpTextFiles} >HelpText.r

acap.r � :include:prefdefs.h :include:stringdefs.h :bits:buildacap
	perl :bits:buildacap �.acap

Credits.r � CreditDefs :bits:buildCredits
	perl :bits:buildCredits CreditDefs > TempCredits.r
	Rez -o TempCredits.rsrc TempCredits.r
	DeRez TempCredits.rsrc "{Rincludes}Pict.r" -D PICT_RezTemplateVersion=0 > Credits.r
	Delete -i TempCredits.r TempCredits.rsrc

#
# action items
#
clean �
	rm -y -i {Derivs} {Eudora} TwoName revname
	cw Eudora.proj; {sendae} -t "{CodeWarrior}" -e MMPRRemB
	ls -F | rmext ~ PreCompPPC .txt .xset .xref .unref .load .MAP .xMAP .segs .tmp .mine .dbg .usf .SYM .xSYM .o .dumpobj .l .acap SYM -carbon -debug -Fat -68k -CFM68K -CFMFat -PPC .doc .sit .makeout
	set exit 0; rm 'spotlight log'� ; rm temp.� ; set exit 1

name � TwoName
	perl -p -e 's/\.//g;s/-.*//;s/^/v/;' TwoName > revname
	p4 label -t CurrentT "MacEud`cat revname`"
	p4 labelsync -l "MacEud`cat revname`"
	p4 label -o "MacEud`cat revname`" | perl -p -e 's/unlocked/locked/' | p4 label -i

icons �
	rm -i icons.rsrc settingsicons.rsrc toolbaricons.rsrc
	makeicons -y 29999 ToolbarIcons ToolbarIcons.rsrc
	makeicons -y 30499 SettingsIcons SettingsIcons.rsrc
	makeicons -y 24999 Icons Icons.rsrc
	rm -i new_icons.rsrc new_settingsicons.rsrc new_toolbaricons.rsrc
	makeicons -y 29999 new_ToolbarIcons new_ToolbarIcons.rsrc
	makeicons -y 30499 new_SettingsIcons new_SettingsIcons.rsrc
	makeicons -y 24999 new_Icons new_Icons.rsrc
	Rez {RezOptions} -t rsrc -c CSOm newiconshell.r -o Eudora60Icons

#
# derive version name from resource file
#
TwoName � 30.rsrc
	dr 30.rsrc -only "'CSOm' (2)" | �
	perl -n -e 'chop;s=.*/\*..== && s=.\*/== && print;' >TwoName
