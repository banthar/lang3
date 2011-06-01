
function write(x:Int):Void;
function read():Int;

function main(argc:Int,argv:**Char):Int
{

	var x:Int=read();

	while(x>0)
	{
		x=x-1;
		write(x);
	}
	
	return x;
	
}

