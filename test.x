#!./lang --run

function writeString(s:*Char):Void;
function writeInt(i:Int):Void;
function read():Int;
function atoi(s:*Char):Int;

type Point = struct
{
	x:Int;
	y:Int;
}

function Point(x:Int, y:Int):Point
{
	var this:Point;
	this.x=x;
	this.y=y;
	return this;
}

function main(argc:Int,argv:**Char):Int
{

	var p:Point;

	atoi("23");


	if(true)
	{
		return read();
	}
	
	return 0;

}

