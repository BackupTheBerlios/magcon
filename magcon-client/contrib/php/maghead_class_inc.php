<?php

/**
 * 
 *
 * @version $0.1$
 * @copyright GPL
 * @author Daniel Preis, 2005, magcon@dpreis.de
 **/
 
 //This Class represents the data structure of the Magellan MapSend Header
 
class maghead {

	var $length;	//char	/*13*/
	var $signature[11];	//char /*"4D533336 MS"*/
	var $version[2];	//char /*"36"*/
	var $type;		//int /*2 -Track*/
	
	function maghead(){
		$length="13";
		$signature="4D533336 MS";
		$version="36";
		$type="2";
}
?>