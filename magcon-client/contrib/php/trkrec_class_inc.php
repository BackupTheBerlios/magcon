<?php

/**
 * 
 *
 * @version $0.1$
 * @copyright GPL
 * @author Daniel Preis, 2005, magcon@dpreis.de
 **/
 
 
include_once "tm_class_inc.php";

//This Class represents the data structure of a Track record
 
 class trkrec
{
	var $x;
	var $y;
	var $altitude;
	var $tempore;
	var $validity;
	var $centisecs;

	function trkrec(){
		$x=0;//double
		$y=0;//double
		$altitude=0;//float
		$tempore=new tm();//time
		$validity=0;//int
		$centisecs="";//char

	}


}

?>