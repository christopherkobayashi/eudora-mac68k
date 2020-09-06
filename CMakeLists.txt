#    Copyright 2015 Wolfgang Thaller.
#
#    This file is part of Retro68.
#
#    Retro68 is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Retro68 is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Retro68.  If not, see <http://www.gnu.org/licenses/>.

# To use this example as a standalone project using CMake:
# mkdir build
# cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# make

cmake_minimum_required(VERSION 3.10)

add_application(Eudora
	acap.c
	address.c
#	adutil.c
#	adwin.c
	aeutil.c
#	anal.c
	appcdef.c
	appear_util.c
	appleevent.c
	audit.c
	auditdefs.c
	binhex.c
	boxact.c
	buildtoc.c
#	carbonutil.c
	color.c
	colorcdef.c
	compact.c
	comp.c
	concentrator.c
	ctb.c
	cursor.c
	dflutils.c
	dial.c
	downloadurl.c
	emoticon.c
	ends.c
	euerrors.c
	export.c
#	extras.c
	featureldef.c
	features.c
	filegraphic.c
	fileutil.c
	fileview.c
	FiltDefs.c
	filters.c
	filt_ldef.c
	filtmng.c
	filtrun.c
	filtthread.c
	filtwin.c
	find.c
	floatingwin.c
	fmtbar.c
	functions.c
	globals.c
	graph.c
	header.c
	hexbin.c
	html.c
	icon.c
	imapauth.c
	imapconnections.c
	imapdownload.c
	imapmailboxes.c
	imapnetlib.c
	import.c
	inet.c
	junk.c
	labelfield.c
	ldaplibglue.c
	ldaputils.c
	ldef.c
	lex822.c
	light.c
	lineio.c
	link.c
	linkmng.c
	linkwin.c
	listcdef.c
	listview.c
	lmgr.c
	log.c
	mailbox.c
	mailxfer.c
	main.c
	makefilter.c
#	mbdrawer.c
	mbwin.c
	md5.c
	menu.c
	menusharing.c
	messact.c
	message.c
	mime.c
	modeless.c
	msiddb.c
	msinfo.c
	msmaildb.c
	mstoc.c
	mstore.c
	multi.c
	mywindow.c
	nag.c
	navUtils.c
#	netscapeplugins.c
	networksetuplibrary.c
	newhtml.c
	nickae.c
	nickexp.c
	nickmng.c
	nickwin.c
	oops.c
	palmconduitae.c
#	paywin.c
	personalitieswin.c
	peteglue.c
	peteuserpane.c
#	pgpin.c
#	pgpout.c
	ph.c
	pop.c
	prefaudit.c
	prefenable.c
	preflimits.c
	prefs.c
	print.c
	progress.c
	proxy.c
	regexp.c
	register.c
	rich.c
#	sasl.c
	schizo.c
	scriptmenu.c
	search.c
	searchwin.c
	sendmail.c
	setldef.c
	shame.c
	signaturewin.c
	sort.c
	speechutil.c
#	spell.c
	spool.c
	squish.c
#	ssl.c
	stationerywin.c
	statmng.c
	statwin.c
	stickypopup.c
	stringutil.c
	sysdefs.c
	table.c
	tabmania.c
	task_ldef.c
	taskProgress.c
	tcp.c
	tefuncs.c
	textw.c
	threading.c
	timebomb.c
	tlate_ldef.c
	toc.c
	toolbar.c
	toolbarpopup.c
	trans.c
	TransStream.c
	unicode.c
	UnloadSeg.c
	updatewin.c
	url.c
	util.c
	utl.c
	uudecode.c
	uupc.c
	vcard.c
	wazoo.c
	winutil.c
	xml.c
    )

# Enable -ffunction-sections and -gc-sections to make the app as small as possible
# On 68K, also enable --mac-single to build it as a single-segment app (so that this code path doesn't rot)
set_target_properties(Eudora PROPERTIES COMPILE_OPTIONS -ffunction-sections)
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    set_target_properties(Eudora PROPERTIES LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-single")

else()
    set_target_properties(Eudora PROPERTIES LINK_FLAGS "-Wl,-gc-sections")
endif()

add_definitions(-DTWO -DACCESSOR_CALLS_ARE_FUNCTIONS)

target_include_directories(Eudora PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/openssl/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CrispinIMAP/Include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Editor/Source/ApplicationHeaders>
#  $<INSTALL_INTERFACE:0iinclude/mylib>  # <prefix>/include/mylib
)
