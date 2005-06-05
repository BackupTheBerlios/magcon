<?php

/**
 * 
 *
 * @version $0.1$
 * @copyright GPL
 * @author Daniel Preis, 2005, magcon@dpreis.de
 **/
 
/*
* This Class represents the functionality of converting *.pdb files
*-extracted with MagCon (www.hansche.de/magcon) from a 
*Magellan GPS-Handheld-Receiver to a PalmOS powered PDA-
*into other data formats.
*/
 

include_once "trkrec_class_inc.php";
include_once "tm_class_inc.php";
include_once "maghead_class_inc.php";


class pdb2text{

//This is the main entry point
//$format can be: "txt"/"csv"/"gpx"
//$data is the data of the *.pdb file, read with the following statement
//$data = file("test.pdb");
function pdb2txt($format,$data){
	$parseresult=array();
	$parseresult=$this->parsemagdatastring($data);
	if($format=="csv"){
		$txtresult= $this->parsemagconfilecsv($parseresult);
	}
	else if($format=="gpx"){
		$txtresult= $this->parsemagconfilegpx($parseresult);
	}
	else if($format=="txt"){
		$txtresult= $this->parsemagconfiletxt($parseresult);
	}
	else if($format=="mapsend"){
		$txtresult= $this->parsemagconfilemapsend($parseresult);//not yet implemented
	}
	//if we like, we can write the content of $txtresult to a file with the following statements:
	/*
	   $filename = 'xyz';
	   $fp = fopen($filename, "w");
	   $write = fputs($fp, $txtresult);
	   fclose($fp);
	*/
	
	//or we just give $txtresult back to the calling function
	return $txtresult;
	
}

//Helper function for converting seperated degree values into one
function mag2degrees($mag_val)
{
        $minutes=0;
        $tmp_val=0;
        $return_value=0;
        $deg=0;
		settype($minutes, "double");
		settype($tmp_val, "double");
		settype($return_value, "double");
		settype($deg, "integer");
		
        $tmp_val = $mag_val / 100.0;
        $deg = (int) $tmp_val;
        $minutes = ($tmp_val - $deg) * 100.0;
        $minutes /= 60.0;
        $return_value = (double) $deg + $minutes;
        return $return_value;
	
} 
//converts the supplied data into mapsend-data
function parsemagconfileimapsend($result){
/*void parsemagconfileimapsend(struct pdb *pdb, const int fd){ 
        $trkrec=new trkrec();
		$maghead=new maghead();
		
        
	char *fname="magcon";
        int j,reccount;
        char buff[PUFFSIZE];
        unsigned char len;
        struct pdb_record *pdb_rec;

        memset(buff,0,PUFFSIZE);
        j=0;
        reccount=0;

        strcat(buff,fname);
        len=strlen(buff);
        write(fd,&hdr,sizeof(struct maghead));
        write(fd,&len,sizeof(len));
        write(fd,buff,len);
        write(fd,&j,sizeof(int));

        for(pdb_rec = pdb->rec_index.rec; pdb_rec; pdb_rec=pdb_rec->next) {

                if(parsemagdatastring((char*)pdb_rec->data,&trk)){
                        write(fd,&trk,sizeof(struct trkrec));
                        reccount++;
                }
        }
        lseek(fd,sizeof(struct maghead)+sizeof(len)+len,SEEK_SET);
        write(fd,&reccount,sizeof(int));*/
}



//convert supplied data into gpx-data
function parsemagconfilegpx($result){
	
	$gpxhead="<?xml version=\"1.0\"?>\n<gpx version=\"1.0\" creator=\"Daniel Preis, Magcon pdb2gpx \" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n<trk>\n<trkseg>\n";
	$gpxfoot="</trkseg></trk></gpx>\n";
	$trkpt="<trkpt lat=\"%f\" lon=\"%f\">\n\t<ele>%f</ele>\n\t<time>%s.%dZ</time>\n</trkpt>\n";
	$reccount=0;
	$resultstring="";
	$parseresult=1;
	
	$resultstring=$gpxhead;

	for($reccount;$reccount<sizeof($result); next($result)) {
	    $trkpoint=new trkrec();
		$trkpoint=current($result);
	
		if($parseresult){
			$formattedtime=strftime("%Y-%m-%dT%H:%M:%S",$trkpoint->tempore);
			$formattedstring=sprintf($trkpt,$trkpoint->y,$trkpoint->x,$trkpoint->altitude,$formattedtime,$trkpoint->centisecs/3*5);
			$resultstring=$resultstring . $formattedstring."\n";
			$reccount++;
		}
	}
	$resultstring=$resultstring . $gpxfoot;

   return $resultstring;

}   

//convert supplied data into comma-seperated-value data
function parsemagconfilecsv($result){
	
	$trkpt="%f,%f,%f,%s.%dZ\n";
	$reccount=0;
	$resultstring="";
	$parseresult=1;
	for($reccount;$reccount<sizeof($result); next($result)) {
	    $trkpoint=new trkrec();
		$trkpoint=current($result);
		if($parseresult){
			$formattedtime=strftime("%Y-%m-%dT%H:%M:%S",$trkpoint->tempore);
			$formattedstring=sprintf($trkpt,$trkpoint->y,$trkpoint->x,$trkpoint->altitude,$formattedtime,$trkpoint->centisecs/3*5);
			$resultstring=$resultstring . $formattedstring."\n";
			$reccount++;
		}
	}
   return $resultstring;

}  

//convert supplied data into array, which contains only longitude and latitude as values
function parsemagconfiletxt($result){
	
	$trkpt="%f,%f,%f,%s.%dZ\n";
	$reccount=0;
	$resultstring = array();
	$parseresult=1;
	for($reccount;$reccount<sizeof($result); next($result)) {
	    $trkpoint=new trkrec();
		$trkpoint=current($result);
		if($parseresult){
			array_push($resultstring,$trkpoint->x);
			array_push($resultstring,$trkpoint->y);
			$reccount++;
		}
	}
   return $resultstring;

}   

//Parses the *.pdb file into data structure of this class.
//$data contains the content of the *.pdb file as seperated per lines
function parsemagdatastring($data){ 
		$result=array();
	foreach ($data as $trk_data => $line) {
            $field=0;
			    $trk_array=explode (",", $line);
		if(strlen($line)>=5){		
			if (substr_compare($line,"PMGNTRK",2,7)==0) {
			
			$trkrec=new trkrec();			
				
				do{
					switch($field){
					case 1:
							$trkrec->y=$this->mag2degrees($trk_array[$field]);
							break;
					case 2:
							if($trk_array[$field]=="N"){
								$trkrec->y=$trkrec->y;
							}
							else {
								$trkrec->y=-$trkrec->y;
							}
							break;
					case 3:
							$trkrec->x=$this->mag2degrees($trk_array[$field]);
							break;
					case 4:
							if($trk_array[$field]=="E"){
								$trkrec->x=$trkrec->x;		
							}
							else {
								$trkrec->x=-$trkrec->x;	
							}
							break;
					case 5:
							$trkrec->altitude=$trk_array[$field];
							break;
					case 7:
							$zeit=new tm();
							sscanf($trk_array[$field],"%2u%2u%2u.%2hu",$zeit->tm_hour,$zeit->tm_min,$zeit->tm_sec,$trkrec->centisecs);
							break;
					case 8:
							$trkrec->validity=!substr_compare($trk_array[$field],"A",0); 
							$ret=1; //?????
							break;
					case 10:
							sscanf($trk_array[$field],"%2u%2u%2u",$zeit->tm_mday,$zeit->tm_mon,$zeit->tm_year);
					 		$trkrec->tempore=mktime($zeit->tm_hour,$zeit->tm_min,$zeit->tm_sec,$zeit->tm_mon,$zeit->tm_mday,$zeit->tm_year,$trkrec->centisecs); 
					 		break;
					default:
					}
				++$field;
				} while($field<sizeof($trk_array));
				array_push($result, $trkrec);
      	
			}
		}
	}
	
	return $result;
}
//helper function for substring compare
function substr_compare($main_str, $str, $offset, $length = NULL, $case_insensitivity = false) {
       $offset = (int) $offset;

       // Throw a warning because the offset is invalid
       if ($offset >= strlen($main_str)) {
           trigger_error('The start position cannot exceed initial string length.', E_USER_WARNING);
           return false;
       }

       // We are comparing the first n-characters of each string, so let's use the PHP function to do it
       if ($offset == 0 && is_int($length) && $case_insensitivity === true) {
           return strncasecmp($main_str, $str, $length);
       }

       // Get the substring that we are comparing
       if (is_int($length)) {
           $main_substr = substr($main_str, $offset, $length);
           $str_substr = substr($str, 0, $length);
       } else {
           $main_substr = substr($main_str, $offset);
           $str_substr = $str;
       }

       // Return a case-insensitive comparison of the two strings
       if ($case_insensitivity === true) {
           return strcasecmp($main_substr, $str_substr);
       }

       // Return a case-sensitive comparison of the two strings
       return strcmp($main_substr, $str_substr);
   }
}
?>