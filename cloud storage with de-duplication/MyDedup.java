/*
*A dropbox-like cloud storage program with de-duplication algo
*using window azure cloud storage service
********/

//package org.myorg;
import java.io.*;
import java.math.BigInteger;
import java.net.URISyntaxException;
import java.util.*;
import java.security.InvalidKeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import com.microsoft.windowsazure.services.core.storage.*;
import com.microsoft.windowsazure.services.blob.client.*;

public class MyDedup {
	private static class Storage
	{
		public static final String storageConnectionString = 
	    	    "DefaultEndpointsProtocol=http;" + 
	    	    "AccountName=portalvhdsc5l6425028n2f;" + 
	    	    "AccountKey=P9zD4uNHqRNir0uDhAHV73N2oE/oC4n7aVV3lLfzQFYm2E8U8Ei9DtUn6rtUFuKimD6x/DHBkoaFH0wpKS/UOw==";
		public CloudStorageAccount storageAccount;
		public CloudBlobClient blobClient;
		public CloudBlobContainer container;
		
		public Map<String, long[]> chunks = new HashMap<String, long[]>();
		public Map<String, List<String>> files = new HashMap<String, List<String>>();
		public File metadata = new File("mydedup.meta");
		public int m; //the window size in bytes
		public long d; //the base parameter
		public int anchor_mask; //2^b - 1
		public int maximum_chunk_size; //on disk storage for chunk buffer 
		public int BUFFER_SIZE;
		public static final long q = 0x100000000L;//Long.parseLong("0x100000000".substring(2), 16); //("100000000", 16); //0x100000000; //2 ^ 32 
		public long d_power_m_mod;  //pre-computed
		public long firstRFP;		//start-point of next RFP
		
		public String tmpPathName = "";
		public List<String> tmpchunksList;
		
		//------------------------Report for upload------------------------------------------
		public double totalChunks = 0;
		public double uniqueChunk = 0;
		public double totalSize = 0; 
		public double dedup_size = 0;
		//------------------------end---------------------------------------------
		//------------------------Report for download------------------------------------------
		public double down_chunk = 0;
		public double down_size = 0;
		public double down_file_size = 0;
		//------------------------Report for delete------------------------------------------
		public double d_chunk = 0;
		public double d_size = 0;
		public Storage()
		{
			//init the cloud storage
			
			try{
				//retrieve cloud storage account
				storageAccount = CloudStorageAccount.parse(storageConnectionString);
			
				//Create blob client
				blobClient = storageAccount.createCloudBlobClient();
			
				// Get a reference to a container
				// The container name must be lower case
				container = blobClient.getContainerReference("yourcontainer");

				// Create the container if it does not exist
				container.createIfNotExist();
			}catch (InvalidKeyException e) {
			// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (URISyntaxException e) {
			// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (StorageException e) {
			// TODO Auto-generated catch block
				e.printStackTrace();
			}
			
			//init metadata file
			if(metadata.exists())
			{//read it if exist else nothing
			try{
				BufferedReader br = new BufferedReader(new FileReader(metadata));
				String line;
				while ((line = br.readLine()) != null) {
					// process the line.
					StringTokenizer tokenizer = new StringTokenizer(line);
					char rowRecord = tokenizer.nextToken().charAt(0);
					switch(rowRecord)
					{
						case 'c': //Chunk Record retrieval
							//if(tokenizer.countTokens()!=3)
								//System.out.println("Format Error on mydedup.meta");
							String checksum = tokenizer.nextToken();	//SHA-1 hash value -> 40 chars
							long size = Long.parseLong(tokenizer.nextToken());
							long refCount = Long.parseLong(tokenizer.nextToken());
							long backend = Long.parseLong(tokenizer.nextToken());
							
							//Map(checksum,pair (size,refCount))
							if(chunks.containsKey(checksum)) {
								System.out.println("Error: duplicate chunks exist");
							} else {
								long[] value = {size, refCount,backend};
								chunks.put(checksum, value);
							}
						break;
						case 'f':  //File Record
							String pathName = tokenizer.nextToken();
							List<String> chunkList = new ArrayList<String>();
							if(tokenizer.hasMoreTokens())
							{
								String tmp = tokenizer.nextToken();
								//Assume there is no ',' in SHA-1 hash value
								String[] chunks = tmp.split(",");
								for(String chunk:chunks)
								{
									chunkList.add(chunk);
								}
							}
							files.put(pathName, chunkList);
						break;
						default:
							System.out.println("Format Error on mydedup.meta");	
					}
				}
				br.close();
				}
				catch(FileNotFoundException e)
				{
					 e.printStackTrace();
				}
				catch(IOException e)
				{
					 e.printStackTrace();
				}
			}
		}
		
		public boolean insertChunkRecord(long backend,String checksum,long chunksize)
		{
			tmpchunksList.add(checksum);  //add to a file record
			if(chunks.containsKey(checksum))
			{//already exist
				long[] value = {chunks.get(checksum)[0], chunks.get(checksum)[1]+1, chunks.get(checksum)[2]};
				chunks.put(checksum, value);
				return false;
			}
			else
			{
				long[] value = {chunksize, 1,backend};
				chunks.put(checksum, value);
				return true;
			}
		}
		public boolean insertFileRecord()
		{
			if(files.containsKey(tmpPathName))
			{
				//files.put(tmpPathName, tmpchunksList);
				System.out.println("Already existing file");
				return false;
			}
			else
			{
				files.put(tmpPathName, tmpchunksList);
				System.out.println("New file created");
				return true;
			}
		}
		public boolean Update()
		{
			 boolean isUpdated = false;
			 try 
			 {
		          BufferedWriter output = new BufferedWriter(new FileWriter(metadata));
				  //BufferedWriter output = new BufferedWriter(new FileWriter(new File("tmp.meta")));
		          for (Map.Entry<String, long[]> item : chunks.entrySet())
		          {
		              output.write("c " + item.getKey() + " " + item.getValue()[0] + " " + item.getValue()[1] + " " + item.getValue()[2]);
		              output.newLine();
		              isUpdated = true;
		          }
		          
		          for (Map.Entry<String, List<String>> item : files.entrySet())
		          {
		              	output.write("f " + item.getKey() + " ");
		  				for(int i=0;i<item.getValue().size(); i++)
		  				{
		  					output.write(item.getValue().get(i));
		  					if(i!=item.getValue().size()-1)
		  						output.write(",");
		  				}
		  				output.newLine();
		  				 isUpdated = true;
		          }
		          
		          //close bufferedWriter -> save file
		          output.close();
		     } 
			 catch ( IOException e ) 
		     {
		           e.printStackTrace();
		     }			 
			 return  isUpdated;
		}
		
		public void set_d_power_m_mod(int m,long d,long q)
		{
			d_power_m_mod = 1;
			while(m > 0){
				if(m % 2 == 1)
					d_power_m_mod = (d_power_m_mod * d) % q;
				m = (m -(m%2)) / 2;
				d = (d * d) % q;
			}
			System.out.println("value d: " + d);
		}
		public void set_firstRFP(byte[] terms,long d, long q)
		{
			int term = (int) terms[0] & 0xFF;
			firstRFP = term;
			for(int i=1; i<terms.length ; i++)
			{
				term = (int) terms[i] & 0xFF;
				firstRFP = firstRFP * d;
				firstRFP = firstRFP % q;
				firstRFP = firstRFP + term;
				firstRFP = firstRFP % q;
			}
			System.out.println("firstRFP: " + firstRFP);
		}
		public long getRFP(byte head, byte tail,long d, long q)
		{
			int headterm = (int) head & 0xFF;
			int tailterm = (int) tail & 0xFF;
			firstRFP = d * firstRFP;
			firstRFP = firstRFP % q;
			firstRFP = firstRFP - ((headterm * d_power_m_mod) % q);
			firstRFP = firstRFP % q;
			firstRFP = firstRFP + tailterm;
			firstRFP = firstRFP % q;
			/*
			long precomputed = d_power_m_mod*headterm%q;
			long result = (d%q*firstRFP%q)%q - precomputed%q;
			result = (result%q + tailterm%q)%q;
			firstRFP = result;  //sequential computation
			*/
			return firstRFP;
		}
		public boolean isInterestedRFP(long RFP)
		{
			int test = (int)RFP & anchor_mask;
			if(test == 0)
			{
				//System.out.println("interested RFP value: " + RFP);
				return true;
			}
			//else
				//System.out.println("RFP value: " + RFP);
			return false;
		}
		public String getChecksum(byte[] data,int chunksize)
		{
			MessageDigest md = null;
			try {
				md = MessageDigest.getInstance("SHA-1");
			} catch (NoSuchAlgorithmException e) {
				e.printStackTrace();
			}
			md.update(data, 0, chunksize);
			byte[] cs= md.digest();
			String cs_string= new BigInteger(1, cs).toString(16);
			return cs_string;
		}
		public void uploadChunk(long backend,byte[] chunk_buf,int chunksize)
		{
			try{
				totalChunks++;
				totalSize+= chunksize;
				//System.out.println("uploadChunk");
				String checksum = getChecksum(chunk_buf,chunksize);
				if(insertChunkRecord(backend,checksum,chunksize))
				{//it is a new chunk, then create a tmp file and upload it to storage
					uniqueChunk++;
					dedup_size+=chunksize;
					if(backend == 1) //remote
					{
						File chunk = new File("chunk");
						FileOutputStream out = new FileOutputStream(chunk);
						out.write(chunk_buf,0,chunksize);
						out.close();
						CloudBlockBlob blob = container.getBlockBlobReference(checksum);
						blob.upload(new FileInputStream(chunk),chunk.length());
					}
					else if (backend == 0) //local
					{
						// Create a directory; all non-existent ancestor directories are
						// automatically created
						File chunkdir = new File("data");
						if (!chunkdir.isDirectory()) 
						{
						    if(!chunkdir.mkdir())
						    	throw new Exception("failure to create data DIR ");
						}
						
						File chunk = new File("data/"+ checksum);
						FileOutputStream out = new FileOutputStream(chunk);
						out.write(chunk_buf,0,chunksize);
						out.close();
					}
				}
			}
			catch(Exception e)
			{
				e.printStackTrace();
			}
		}
		public long chunking(File file,long backend)
		{
			long RFPCount = 0; //including 0 
			try{
				//Temp chunk
				byte[] chunk_buf = new byte[ maximum_chunk_size];
				int backup=0;
				
				//file block buffer
				byte[] buf = new byte[BUFFER_SIZE];
				byte[] buf2 = new byte[BUFFER_SIZE];
				//window buffer
				byte[] terms = new byte[m];
				
				//offset parameter
				int start=0,tail=0;
				int RFPhead=0,RFPlength=0;
				int chunkhead=0;
				
				int bytesRead = 0;
				FileInputStream in = new FileInputStream(file);
				
				boolean isStart = true;
				
				//shift       
				int shift = 0;
				
				while((bytesRead = in.read(buf, shift, BUFFER_SIZE-shift)) > 0){
					//read a block of data to buf
					//process one block
					if(bytesRead + shift > m) // at least m + 1
					{
						if(isStart)
						{
							start = 0;
							RFPhead = 0;
							System.arraycopy(buf, 0, terms, 0, m);  //copy one window to terms
							//pre-compute the main factor(p0 and d^m mod q)
							set_d_power_m_mod(m,d,q);
							set_firstRFP(terms,d,q);
							//check if it is anchor point
							if(isInterestedRFP(firstRFP))
							{
								RFPCount++;
								RFPlength = m;
								System.arraycopy(buf,RFPhead,chunk_buf,chunkhead, RFPlength);
								uploadChunk(backend,chunk_buf,RFPlength);
								RFPhead += RFPlength;
								chunkhead=0; //reset to zero 
								//System.out.println("Interested on first");
							}
							
							if(bytesRead + shift == m + 1)
							{
								RFPlength = m - RFPhead + 1 ;
								System.arraycopy(buf,RFPhead,chunk_buf,chunkhead, RFPlength);
								uploadChunk(backend,chunk_buf,RFPlength);
								System.out.println(" file end");
								break;  //end of file
							}
							
							isStart = false;
						}
						
						System.out.println("m:"+m);
						boolean z = true;
						for(int i=1; i <= bytesRead + shift - m; i++)  //end until the last m window
						{	
							//----------------Check if it is interested RFP------------------------
							//for current tail
							tail = (i-1) + m ; //term t for s + m , it is the last term of current window
							long RFP = getRFP(buf[start],buf[tail],d,q);
							
							//System.out.print("|"+i +","+start +"," + tail +"|");
							
							RFPlength = (tail - RFPhead) + 1;
							if(isInterestedRFP(RFP) | RFPlength == maximum_chunk_size)
							{
								if(RFPlength >= m) //Minimum chunk size requirement
								{
									RFPCount++;
									//System.out.println("interested " + i +":"+ RFP +","+ (tail+1));
									System.arraycopy(buf,RFPhead,chunk_buf,chunkhead, RFPlength); //flush out
									uploadChunk(backend,chunk_buf,RFPlength);
									RFPhead += RFPlength;
									chunkhead=0; //reset to zero
								}
							}
							
							start = i; //for next start 
							//----------------------end--------------------------------------------	
							//shift if facing the end of buf
							if(bytesRead + shift - m - 1 == i)
							{
								System.out.println("flush out");
								if((RFPlength = start - RFPhead) > 0) 
								{
									//System.out.println("positive:" + i +"," + RFPlength);
									System.arraycopy(buf,RFPhead,chunk_buf,chunkhead, RFPlength); //flush out
									chunkhead += RFPlength;
									backup += RFPlength;
									RFPhead = 0;
								}
								else//RFPlength will be negative
								{
									//System.out.println("negative");
									RFPhead = RFPhead - start;
								}
								//System.arraycopy(buf,0,buf2,0,)
								System.arraycopy(buf,i,buf,0, m+1); //shift the last window m + 1to the beginning of the buf
								start = 0;
								tail = 0;
								shift = m + 1; //it will shift in the next round, start + (window..tail)  -> so the minmum size for next round must be >= m + 1
								break;
							}
						}
						System.out.println("next round");
					}
					else
					{// file size <= m 
							uploadChunk(backend,buf,bytesRead);
							//will break due to the end of file
					}
				}
				
				
				if(shift!=0)
				{
					//flush the remaining bytes
					System.out.println("Total size before remain:" + totalSize);
					RFPlength = m - RFPhead + 1; //don't miss head!!!
					
					System.arraycopy(buf,RFPhead,chunk_buf,chunkhead, RFPlength); //flush out
					uploadChunk(backend,chunk_buf,RFPlength + backup);
					
					System.out.println("flush the remaining bytes: "+ backup + "|" + RFPlength);
				}
				in.close();
			}
			catch(Exception e)
			{
				e.printStackTrace();
			}
			return RFPCount;
		}
		public boolean upload(String m, String d, String b, String x,String pathName,String backend) throws Exception
		{
			//init parameters
			this.m = Integer.parseInt(m);
			this.d = Long.parseLong(d);
			this.anchor_mask = 1;
			for(int i=0;i<Integer.parseInt(b);i++){
				this.anchor_mask  = this.anchor_mask  << 1;
			}
			this.anchor_mask  = this.anchor_mask  - 1;
			this.maximum_chunk_size = Integer.parseInt(x);
			
			//changed!!!!!!!!!!!!!!!!!!!!!!!!!
			if(this.maximum_chunk_size > 1048576)
				this.BUFFER_SIZE = maximum_chunk_size*4;
			else
				this.BUFFER_SIZE = 1048576*4; //4MB
			
			System.out.println("BUFFER_SIZE: " + this.BUFFER_SIZE);

			//Create a file record for upload
			tmpPathName = pathName;
			tmpchunksList = new ArrayList<String>();
			
			//backend
			long backtmp = 0;
			if(backend.equals("local"))
				backtmp = 0;
			else if(backend.equals("remote"))
				backtmp = 1;
			else
				throw new Exception("local or remote");
			
			//chunking
			File file = new File(tmpPathName);
			if(insertFileRecord())
				System.out.println("No. of anchor points"+chunking(file,backtmp));
			return true;
		}
		public boolean getChunk(String checksum,String pathName,byte[] buf) throws Exception 
		{
			if(chunks.containsKey(checksum))
			{
				if(chunks.get(checksum)[2]==1) //download from remote
				{
					try
					{
						CloudBlockBlob blob = container.getBlockBlobReference(checksum);
						blob.download(new FileOutputStream(pathName,true));
					}catch(Exception e){
						e.printStackTrace();
					}
				}
				else if(chunks.get(checksum)[2]==0) //download from local
				{
					FileInputStream chunkIn;
					FileOutputStream fileIn;
					long readlen,writelen;
					try {
						chunkIn = new FileInputStream( "data/" + checksum);

						fileIn = new FileOutputStream(pathName,true); //from the end to write
						
						//read one chunk from backend
						if((int)chunks.get(checksum)[0]>1048576)
							buf = new byte[(int)chunks.get(checksum)[0]];
						if((readlen = chunkIn.read(buf, 0, (int)chunks.get(checksum)[0])) != chunks.get(checksum)[0])
						{
							chunkIn.close();
							fileIn.close();
							throw new Exception("download error");
						}
						
						
						//write one chunk into file of pathName
						fileIn.write(buf,0,(int)chunks.get(checksum)[0]);
						chunkIn.close();
						fileIn.close();
					}catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
					
				return true;
			}
			return false;
		}
		public boolean download(String pathName) throws Exception
		{
			if(files.containsKey(pathName))
			{
				File tmp = new File(pathName);
				if(tmp.exists())
					tmp.delete();
                
				Map<String, Long> tmp_m = new HashMap<String, Long>();

				byte[] buf = new byte[1048576];  //hardcoded!!!!!!!!!!!!!!!!!!1
				for(int i=0;i<files.get(pathName).size();i++)
					if(!getChunk(files.get(pathName).get(i),pathName,buf))
					{
						System.out.println("missing chunks");
						return false;
					}
					else
					{
						tmp_m.put(files.get(pathName).get(i), chunks.get(files.get(pathName).get(i))[0]);
						down_file_size += chunks.get(files.get(pathName).get(i))[0];
				    }
				down_chunk = tmp_m.size();  //count the number of unique chunk
				for(Map.Entry<String,Long> item : tmp_m.entrySet()) //count the total size of unique chunks
				{
					//System.out.println("value" +item.getValue());
					down_size += item.getValue();
				}
				return true;
			}
			return false;
		}
		public boolean delete(String pathName)
		{
			if(files.containsKey(pathName))
			{
				//decrease all chunksum's refCount
				for(int i=0;i<files.get(pathName).size();i++)
				{
					if(chunks.containsKey(files.get(pathName).get(i)))
					{
						chunks.get(files.get(pathName).get(i))[1]--; //decrease the refCount
						//remove the chunk record if its refCount = 0
						if(chunks.get(files.get(pathName).get(i))[1]==0)
						{
						    d_chunk++; //total number of chunks deleted
						    d_size += chunks.get(files.get(pathName).get(i))[0]; // count the total size of deletion
							//remove the chunk file from backend
							if(chunks.get(files.get(pathName).get(i))[2]==1)  //from remote
							{
								try {
									CloudBlockBlob blob;
									blob = container.getBlockBlobReference(files.get(pathName).get(i));
									blob.delete();
								} catch (URISyntaxException e) {
									// TODO Auto-generated catch block
									e.printStackTrace();
								} catch (StorageException e) {
									// TODO Auto-generated catch block
									e.printStackTrace();
								}
							}
							else if(chunks.get(files.get(pathName).get(i))[2]==0) //from local
							{
								if (!(new File("data/" + files.get(pathName).get(i))).delete())
									System.out.println("Error: delete file");
							}
							
							//remove the chunk record from the in-memory index
							chunks.remove(files.get(pathName).get(i));
						}
					}
				}
				//remove the file record from metadata
				files.remove(pathName);
				return true;
			}
			return false;
		}
	}
	public static void main(String[] args) throws Exception
	{
			
			//set JVM proxy to use CSE proxy 
		    System.setProperty("http.proxyHost", "proxy.cse.cuhk.edu.hk");
			System.setProperty("http.proxyPort", "8000");
			Storage cloud = new Storage();

			
			if ( args.length == 7 && args[0].equals("upload") ){
				if(cloud.upload(args[1],args[2],args[3],args[4],args[5], args[6]) )
				{
					System.out.println("-------------------------Uploaded---------------------------------");
					System.out.println("Report Output:");
					System.out.println("Total number of chunks: " + cloud.totalChunks);
					System.out.println("Number of unique chunks: (no. of chunks uploaded for this file)"+ cloud.uniqueChunk);
					System.out.println("Number of bytes with deduplication(uploaded):"+ cloud.dedup_size);
					System.out.println("Number of bytes without deduplication(file size):"+ cloud.totalSize);
					System.out.printf("Deduplication ratio: %.2f", cloud.dedup_size/cloud.totalSize);
					System.out.println("");
				}
			} else if (args.length == 2 && args[0].equals("download")){
				if(cloud.download(args[1]))
				{
					System.out.println("-------------------------Downloaded---------------------------------");
					System.out.println("Report Output:");
					System.out.println("Number of chunks downloaded:" + cloud.down_chunk);
					System.out.println("Number of bytes downloaded:" + cloud.down_size);
					System.out.println("Number of bytes reconstructed:" + cloud.down_file_size);
				}
				else
					System.out.println("Download Failure!");
			} else if (args.length == 2 && args[0].equals("delete")){
				if(cloud.delete(args[1]))
				{
					System.out.println("-------------------------Deleted---------------------------------");
					System.out.println("Report Output:");
					System.out.println("Number of chunks deleted:" + cloud.d_chunk);
					System.out.println("Number of bytes deleted:" + cloud.d_size);
				}
			} else if (args.length == 1 && args[0].equals("show")){
				System.out.println("-------------------------show file stored---------------------------------");
				for(String file:cloud.files.keySet())
					System.out.println(file);
			}else {
				System.out.println("Usage:");
				System.out.println("For file upload: upload m d b x <file_to_upload> <local/remote>");
				System.out.println("For file download: download <file_to_download>");
				System.out.println("For file deletion: delete <file_to_delete>");
				System.out.println("For file exploration: show");
			}
			
			if(cloud.Update())
				System.out.println("Refresh the metadata file");
			else
				System.out.println("Empty metadata file created");

	}
}
