#!./lang

function writeString(s:*Char):Void;
function writeInt(i:Int):Void;
function read():Int;
function atoi(s:*Char):Int;

function main(argc:Int,argv:**Char):Int
{
	var i:Int=0;
	while(i<argc)
	{
		writeString(argv[i]);
		i++;
	}
	return 0;
}

