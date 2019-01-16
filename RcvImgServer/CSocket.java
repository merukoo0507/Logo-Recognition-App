import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import org.omg.CORBA.portable.InputStream;

public class CSocket implements Runnable {
	String Filename;
	StringBuffer rcvBuffer=new StringBuffer();
	
	CSocket(String filename)
	{
		Filename = filename;
	}

	@Override
	public void run() {
		// TODO Auto-generated method stub
		Socket client;
		try {
			client = new Socket(InetAddress.getByName("localhost"), 1234);
		
			java.io.InputStream in=client.getInputStream();
			BufferedOutputStream bos = new BufferedOutputStream( client.getOutputStream() );
			
			System.out.println("send string: "+Filename);
			bos.write( Filename.getBytes() ); // writes to the buffer
			bos.flush() ; // writes the buffer to the network
			
			while(true){
				int x=in.read();
				if ((char) ((byte) x)=='#') 
					break;
				rcvBuffer.append((char) ((byte) x));
			}
			client.close();
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public String getRcvString()
	{
		return rcvBuffer.toString();
	}
	
}
