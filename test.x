#!./lang

function writeString(s:*Char):Void;
function writeInt(i:Int):Void;
function read():Int;
function atoi(s:*Char):Int;

function main(argc:Int,argv:**Char):Int
{

	for(var i:Int=0;i<argc;i++)
	{
		writeString(argv[i]);
	}

	return 0;

}

