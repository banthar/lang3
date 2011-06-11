#!./lang

function writeString(s:*Char):Void;
function writeInt(s:*Char):Void;
function read():Int;
function atoi(a:*Char):Int;

function main(argc:Int,argv:**Char):Int
{
	var tmp:*Char="XXX";
	tmp[0]='x';
	writeString(tmp);
	return 0;
}

