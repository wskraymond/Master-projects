/*
*A program used to find a shortest path with a network with billions of nodes (Big Data problem)
*Using distributed platform (Hadoop) , by its map-reduce programming & parallel breath-first search algo
*Infrastructure: (IaaS)4 VM (one master node, others are slave nodes) of open stack platform (Open Source Cloud Computing Software)
*Distributed Computing Platform: Hadoop set up on 4 VM (can be more)
****************/

package org.myorg;

import java.io.IOException;
import java.util.*;

import org.apache.hadoop.fs.Path;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.*;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat;

//import org.apache.hadoop.mapred.Counters;

public class ParallelBfs{
	public final static int INFINITE = 100000;
	public static int sourceNode;
	public static boolean first=true;
	public static String adjacentList;
	public static long track;
	public static enum Color{
		WHITE,GRAY,BLACK
	};
	
	public static enum GrayCounter {
		COUNT
	};
	
	enum WordCount 
	{ 
		NUM_OF_TOKENS
		,NUM_OF_IN,
		NUM_OF_OUT
	}
	
	public static int round = 0;
	public static int getColorIndex(Color c)
	{
		switch(c)
		{
			case WHITE:
				return 0;
			case GRAY:
				return 1;
			case BLACK:
				return 2;	
		}
		return -1;
	}
		
	public static class Node
	{
		public int nid;
		public int distance;
		public Color color;
		public List<Integer> list;
		public String getColor()
		{
			switch(color)
			{
				case WHITE:
					return "WHITE";
				case GRAY:
					return "GRAY";
				case BLACK:
					return "BLACK";	
			}
			return "COLOR";
		}
		public void setColor(String c)
		{
			if(c.equals("WHITE"))
				color = Color.WHITE;
			else if(c.equals("GRAY"))
				color = Color.GRAY;
			else if(c.equals("BLACK"))
				color = Color.BLACK;
		}
		public Node()
		{
			//nothing
		}
		public Node(int nid,  List<Integer> list,int distance, Color color)
		{
			this.nid = nid;
			this.distance = distance;
			this.color = color;
			if(list==null)
			{
				this.list = new ArrayList<Integer>();
				this.list.add(-1);
			}
			else
				this.list = list;
		}
		
		public Node(String str)
		{
			this.list = new ArrayList<Integer>();
			String[] tokens = str.split("\\s+");
			nid = Integer.parseInt(tokens[0]);
			System.out.println("tokens[0]:|"+ tokens[0] + "|");
			String[] items = tokens[1].split("/");
			System.out.println("tokens[1]:|"+ tokens[1] + "|");
			System.out.println("items[0]:|"+ items[0] + "|");
			System.out.println("items[1]:|"+ items[1] + "|");
			System.out.println("items[2]:|"+ items[2] + "|");
			String[] nodes = items[0].split(",");
			for(String node:nodes)
			{
				System.out.println("node:|"+ node + "|");
				list.add(Integer.parseInt(node));
			}
			System.out.println("distance:|"+ items[1] + "|");
			System.out.println("color:|"+ items[2] + "|");
			distance = Integer.parseInt(items[1]);
			setColor(items[2]);
		}		
		public String export()
		{
			String tmp="";
			for(int i=0;i<list.size(); i++)
			{
				tmp += Integer.toString(list.get(i));
				if(i!=list.size()-1)
					tmp+= ",";
			}
			tmp += "/";
			
			tmp += Integer.toString(distance) + "/";
			tmp += getColor();
			return tmp;
		}
	}
	
	public static class Map_parser extends 
			Mapper<LongWritable, Text, IntWritable, IntWritable> {
		
		//Use data structure to store key-value pair for output
		//since the mapper output is to the buffer(memory) which is used for other operation before on disk
		//Hadoop cannot handle too many emit pairs
		//Does it mean that we need to do In-Mapper combining to reduce the number of emit times ???
		//don't use hashmap here, not unique.
		private List<IntWritable> keyList;	
		private List<IntWritable> valueList;
		//Why need flush -> in-mapper memory is limited
		//out of size -> have to flush -> emit
		private static final int FLUSH_SIZE = 1000;
		private void flush(Context context, boolean force) throws IOException, InterruptedException {
			if(keyList.size()!=valueList.size())
				System.out.println("ERROR!!!!!!!!!!!!!");
			if(!force) {
				int size = keyList.size();
				if(size < FLUSH_SIZE)
					return;
			}
			
			for (int i=0; i<keyList.size();i++){
            			context.write(keyList.get(i),valueList.get(i));
			}
			System.out.println("Flush");
			//clear
			keyList.clear();	
			valueList.clear();	
		}
		
		public void setup(Context context){
			System.out.println("setup");
			keyList = new ArrayList<IntWritable>();
			valueList = new ArrayList<IntWritable>();
		}
		public void map(LongWritable key, Text value, Context context)
				throws IOException, InterruptedException {
				//obviously, key is not used here.
			
			IntWritable start = new IntWritable();
			IntWritable end = new IntWritable();
				
			//line by line input
			String line = value.toString(); //<Text class obj>.toString() -> convert byte format to String
			StringTokenizer tokenizer = new StringTokenizer(line);
			
			if(tokenizer.hasMoreTokens()) 
				start.set(Integer.parseInt(tokenizer.nextToken()));  //<hadoop type class obj>.set(<value>);
			if(tokenizer.hasMoreTokens()) 
				end.set(Integer.parseInt(tokenizer.nextToken()));
			
			context.getCounter(WordCount.NUM_OF_TOKENS).increment(1);
			//put in Both-list
			keyList.add(start);
			valueList.add(end);
			System.out.println("add");
			//whether need to flush
			flush(context, false);
		}
		
		protected void cleanup(Context context) throws IOException, InterruptedException {
			flush(context, true);
			System.out.println("cleanup");
		}
	}
	
	public static class Reduce_parser extends
			Reducer<IntWritable, IntWritable, IntWritable, Text> {
		public void setup(Context context){
			//get the argument of source node
			Configuration conf = context.getConfiguration();
			sourceNode = Integer.parseInt(conf.get("sourceNode"));
		}
		public void reduce(IntWritable key, Iterable<IntWritable> values,
				Context context) throws IOException, InterruptedException {
			List<Integer> list = new ArrayList<Integer>();
			for (IntWritable val : values) 
			{
				list.add(val.get());
			}
			
			Node N;
			if(key.get()==sourceNode)
				 N = new Node(key.get(),list,0,Color.WHITE);
			else
				N = new Node(key.get(),list,INFINITE,Color.WHITE);  //-1 for infinite
			context.write(key, new Text(N.export()));  //since sum is int type , not class object IntWritable
		}
	}
	
	public static class Map_Iteration extends Mapper<LongWritable, Text, IntWritable, Text> 
	{
		//private static final int FLUSH_SIZE = 1000;
		/*
		private void flush(Context context, boolean force) throws IOException, InterruptedException {
			if(keyList.size()!=valueList.size())
				System.out.println("ERROR!!!!!!!!!!!!!");
			if(!force) {
				int size = keyList.size();
				if(size < FLUSH_SIZE)
					return;
			}
			for (int i=0; i<keyList.size();i++){
            			context.write(keyList.get(i),valueList.get(i));
			}
		}
		*/
		public void setup(Context context){
			System.out.println("setup");
		}
		public void map(LongWritable key, Text value, Context context)
				throws IOException, InterruptedException {
				//obviously, key is not used here.
			System.out.println("map iterate start");
			Node N = new Node(value.toString());
			if(N.nid == sourceNode)
				N.color = Color.GRAY;  //starting point!!!!!!!!!!!
			switch(N.color)
			{
				case BLACK: case WHITE:
					System.out.println("emit-> " + N.nid + " " + N.export());
					context.write(new IntWritable(N.nid),new Text(N.export())); //emit itself
					break;
				case GRAY:
					N.color = Color.BLACK;
					System.out.println("emit-> " + N.nid + " " + N.export());
					context.write(new IntWritable(N.nid),new Text(N.export())); //emit itself as black
					Node neighbour;
					int d;
					if(!(N.list.get(0)==-1))
					{
					for(int neighbour_id : N.list)
					{
						if(N.distance==INFINITE)
							d = INFINITE; //still infinite
						else
							d = N.distance + 1; // plus one, take one step for search frontier, if it is sourceNode, then N.distance = 0
						neighbour = new Node(neighbour_id,null,d,Color.GRAY);	//without list
						System.out.println("emit-> " + neighbour.nid + " " + neighbour.export());
						context.write(new IntWritable(neighbour.nid), new Text(neighbour.export())); //emit neighbors with gray
					}
					}
					break;
				default:
					System.out.println("Error on mapper_iteraion");
			}
			System.out.println("map iterate end");
			
			//whether need to flush
			//flush(context, false);
		}
		
		protected void cleanup(Context context) throws IOException, InterruptedException {
			//flush(context, true);
			//System.out.println("cleanup");
		}
	}
	
	public static class Reduce_Iteration extends Reducer<IntWritable, Text, IntWritable, Text> {
		public void reduce(IntWritable key, Iterable<Text> values, Context context) throws IOException, InterruptedException{
			System.out.println("redunce iterate start");
			Node tmp;
			int min = INFINITE;
			int c_min =  getColorIndex(Color.WHITE);
			Node outNode = new Node(-1,null,INFINITE,Color.WHITE);
			outNode.nid = key.get();
			
			//combine the Node info
			//?
			for (Text val : values) 
			{
				tmp = new Node(Integer.toString(outNode.nid) + " "+ val.toString());
				
				//list
				if(tmp.list.get(0)!=-1)
					outNode.list = tmp.list; 
				
				//distance
				if(tmp.distance < min) //Black is min ??
				{
					min = tmp.distance;
					outNode.distance = min;
				}
				
				//color
				if(getColorIndex(tmp.color) > c_min) 
				{
					c_min = getColorIndex(tmp.color);
					outNode.color = tmp.color;
				}
				
			}
			//increment counter for nodes with Gray color 
			if(outNode.color==Color.GRAY)
			{
				context.getCounter(GrayCounter.COUNT).increment(1);
				System.out.println("COUNTER FUCK------------------------------------------------------");
			}
			//then emit
			System.out.println("emit-> " + outNode.nid + " " + outNode.export());
			context.write(new IntWritable(outNode.nid), new Text(outNode.export()));
			System.out.println("redunce iterate end");
			//when tp break out of Iteration;
		}
	}
	
	public static class Map_output extends Mapper<LongWritable, Text, IntWritable, IntWritable> 
	{
		public void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
				//obviously, key is not used here.
				Node N = new Node(value.toString());
				if(N.color == Color.BLACK)
					context.write(new IntWritable(N.nid),new IntWritable(N.distance));
		}
	}
	public static class Reduce_output extends Reducer<IntWritable, IntWritable, IntWritable, IntWritable> {
		public void reduce(IntWritable key, Iterable<IntWritable> values, Context context) throws IOException, InterruptedException {
			for (IntWritable val : values) 
			{
				context.write(key, val);  // val.get() -> return int type value
			}
		}
	}
	
	public static String input_parsing(Job job, String input) throws Exception
	{		
		String out = input + "-input";
		FileInputFormat.addInputPath(job, new Path(input));  // get the input path from command
		FileOutputFormat.setOutputPath(job, new Path(out)); // get the output path from command

		if(job.waitForCompletion(true))
		{
			System.out.println("Input Parsing Succeeed");
			return out;
		}
		else
			throw new Exception("Error on Input Parsing");
	}
	
	public static Job iteration(Configuration conf) throws Exception
	{
		Job job2 = new Job(conf, "Iteration" + round);
		job2.setOutputKeyClass(IntWritable.class);
		job2.setOutputValueClass(Text.class);
		
		//set Jar Archive
		job2.setJarByClass(ParallelBfs.class);
		
		job2.setMapperClass(Map_Iteration.class);
		//job.setCombinerClass(Reduce_parser.class);
		job2.setReducerClass(Reduce_Iteration.class);

		job2.setInputFormatClass(TextInputFormat.class);		//Text type
		job2.setOutputFormatClass(TextOutputFormat.class);	//Text type
		
		FileInputFormat.addInputPath(job2, new Path(adjacentList));  // get the input path from command
		adjacentList = adjacentList + round;
		FileOutputFormat.setOutputPath(job2, new Path(adjacentList)); // get the output path from command

		if(job2.waitForCompletion(true))
		{
			System.out.println("Iteration round "+ round +" Succeeed");
			round++;
			return job2;
		}
		else
			throw new Exception("Error on Iteration round " + round );
	}
	
	public static void main(String[] args) throws Exception {
		// Run on a pseudo-distributed node 
		Configuration conf = new Configuration();
		//conf.set("fs.default.name", "file:///");
		//conf.set("mapred.job.tracker", "local");
		conf.set("sourceNode", args[2]);
		
		Job job1 = new Job(conf, "Input Parsing");
		job1.setOutputKeyClass(IntWritable.class);
		job1.setOutputValueClass(IntWritable.class);
		
		//set Jar Archive
		job1.setJarByClass(ParallelBfs.class);
		
		job1.setMapperClass(Map_parser.class);
		//job.setCombinerClass(Reduce_parser.class);
		job1.setReducerClass(Reduce_parser.class);

		job1.setInputFormatClass(TextInputFormat.class);		//Text type
		job1.setOutputFormatClass(TextOutputFormat.class);	//Text type
		
		System.out.println("Input parsing");
		//input parsing
		adjacentList = input_parsing(job1,args[0]);
		System.out.println("Input parsing end");
	
		//iteration
		System.out.println("Iteration start");
		while(true)
		{
			System.out.println("INPUT PATH:" + adjacentList);
			Job job2 = iteration(conf);  //output to input iteration
			track=job2.getCounters().findCounter(ParallelBfs.GrayCounter.COUNT).getValue();
			System.out.println("track:"+track);
			if(track==0)
				break;	
		}
		System.out.println("Iteration end");
		
		//Output conversion
		Job job3 = new Job(conf, "Output");
		job3.setOutputKeyClass(IntWritable.class);
		job3.setOutputValueClass(IntWritable.class);
		
		//set Jar Archive
		job3.setJarByClass(ParallelBfs.class);
		
		job3.setMapperClass(Map_output.class);
		//job.setCombinerClass(Reduce_parser.class);
		job3.setReducerClass(Reduce_output.class);

		job3.setInputFormatClass(TextInputFormat.class);		//Text type
		job3.setOutputFormatClass(TextOutputFormat.class);	//Text type
		FileInputFormat.addInputPath(job3, new Path(adjacentList));  // get the input path from command
		FileOutputFormat.setOutputPath(job3, new Path(args[1])); // get the output path from command
		job3.waitForCompletion(true);
		System.out.println("Total round: " + round);
	}
}
