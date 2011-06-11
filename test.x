#!./lang

function writeString(s:*Char):Void;
function writeInt(i:Int):Void;
function read():Int;
function atoi(s:*Char):Int;

type Point = struct
{
	x:Int;
	y:Int;
}

function main(argc:Int,argv:**Char):Int
{

	var p:Point;
	p.x=0;
	p.y=1;
	
	writeInt(p.y);

	return 0;

}

