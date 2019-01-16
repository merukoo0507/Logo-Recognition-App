import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;

import org.omg.CORBA.portable.InputStream;

public class JServer implements Runnable {
	public static final String file_name = "C:\\Merukoo\\Logo-Recognition-App\\RcvImgServer\\test";
	public static final int SERVERPORT = 168, SENDPORT = 268;
	Socket client, send2Client;
	
	@Override
	public void run() {
		// TODO Auto-generated method stub
		System.out.println("Server: connecting");
		try {
			ServerSocket serverSocket = new ServerSocket(SERVERPORT);
			ServerSocket sendSocket = new ServerSocket(SENDPORT);
			while(true){
				client = serverSocket.accept();
				send2Client = sendSocket.accept();

				System.out.println("=========================================");
				System.out.println("Server: Receiving.");
				
				OutputStream fOut = new FileOutputStream(file_name+".jpg");
				byte[] buf = new byte[1024];
				int len;
				
				DataInputStream dIn = new DataInputStream(client.getInputStream());
				while((len = dIn.read(buf)) != -1){
					fOut.write(buf, 0, len);
				}
				System.out.println("Done");

				CSocket cs = new CSocket(file_name+".jpg");
				cs.run();
				System.out.println("Answer:"+cs.getRcvString());
				
				DataOutputStream dOut = new DataOutputStream(send2Client.getOutputStream());
				dOut.writeUTF(cs.getRcvString());
//				dOut.writeUTF("Answer");
//				System.out.println("Answer");
				
				dIn.close();
				dOut.close();
				fOut.flush();
				fOut.close();
				client.close();
				send2Client.close();
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			System.out.println("Server: error.");
			e.printStackTrace();
		}
	}

}
