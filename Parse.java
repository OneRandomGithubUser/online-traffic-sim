import java.util.Scanner; 
import java.io.PrintWriter;
import java.io.FileInputStream; 
import java.io.IOException;
import java.io.FileWriter;
import java.util.UUID;
import java.util.ArrayList;
import java.util.HashMap;

public class Parse{
   
   //key (short uuid), value (long uuid)
   static HashMap<String, String> uuidMap = new HashMap<String, String>();
   
   public static void print(String str){
      System.out.println(str);
   }
   
   public static int parseNodes(String fileName) throws IOException {
      FileInputStream is = new FileInputStream(fileName);
      Scanner inSS = new Scanner(is);
      int count = 0;
      while(inSS.hasNextLine()){
         String ln = inSS.nextLine();
         if (ln.contains("node") && !ln.contains("/>")) {
            boolean isHighway = false;
            String nxLn = inSS.nextLine();
            while(!nxLn.contains("/node")) {
               if (nxLn.contains("highway")) {
                  count++;
                  break;
               }
               nxLn = inSS.nextLine();
            }
         }
      }
      Node[] Nodes = new Node[count];
      print(""+count);
      
      is = new FileInputStream(fileName);
      inSS = new Scanner(is);
      int ind = 0;
      while(inSS.hasNextLine()){
         String ln = inSS.nextLine();
         if (ln.contains("node") && !ln.contains("/>")) {
            
            boolean isHighway = false;
            String nxLn = inSS.nextLine();
            while(!nxLn.contains("/node")) {
               if (nxLn.contains("highway")) {
                  isHighway = true;
                  break;
               }
               nxLn = inSS.nextLine();
            }
            
            if (isHighway) {
               String id = "";
               if (ln.contains("id")) {
                  int ind1 = ln.indexOf("id") + 4;
                  for (int i = ind1; i < ln.length(); i++) {
                     if (ln.charAt(i)=='"'){
                        break;
                     }
                     id += ln.charAt(i);
                  }
               }
               else {
                  continue;
               }
               
               String uuid = UUID.randomUUID().toString();
               
               String lat = " ";
               if (ln.contains("lat")) {
                  int ind2 = ln.indexOf("lat") + 5;
                  for (int i = ind2; i < ln.length(); i++) {
                     if (ln.charAt(i)=='"'){
                        break;
                     }
                     lat += ln.charAt(i);
                  }
               }
               
               String lon = " ";
               if (ln.contains("lon")) {
                  int ind3 = ln.indexOf("lon") + 5;
                  for (int i = ind3; i < ln.length(); i++) {
                     if (ln.charAt(i)=='"'){
                        break;
                     }
                     lon += ln.charAt(i);        
                  }   
               }
               
               Nodes[ind] = new Node(uuid, id, lat, lon);
               uuidMap.put(id, uuid);
               ind++;
            }
         }
      }
      
      PrintWriter outfile = new PrintWriter("Nodes.json");
      outfile.println("[");
      for (int i = 0; i < ind; i++) {
         outfile.println(" {");
         outfile.println("    \"uuid\": \""+Nodes[i].uuid+"\",");
         outfile.println("    \"x\": \""+Nodes[i].x+"\",");
         outfile.println("    \"y\": \""+Nodes[i].y+"\"");
         String temp = " },";
         if (i == ind - 1) {
            temp = " }";
         }
         outfile.println(temp);
      }
      outfile.println("]");
      outfile.flush();
      outfile.close();      
      return ind;
   }
   
   public static boolean inList(String[] list, String target) {
      for (int i = 0; i < list.length; i++) {
         if (list[i].contains(target)) {
            return true;
         }
      }
      return false;
   }
   
   public static void parseWays(String fileName, String nodeFile, int index) throws IOException {
      
      FileInputStream is = new FileInputStream(fileName);
      Scanner inSS = new Scanner(is);
      int count = 0;
      
      while(inSS.hasNextLine()){
         String ln = inSS.nextLine();
         if (ln.contains("<way") && !ln.contains("/>")) {
            count++;
         }
      }
      
      Way[] Ways = new Way[count];
      System.out.println(count);
      int ind = 0;
      
      is = new FileInputStream(fileName);
      inSS = new Scanner(is);
      
      while(inSS.hasNextLine()){
         String ln = inSS.nextLine();
         if (ln.contains("<way") && !ln.contains("/>")) {
            String id = "";
            if (ln.contains("id")) {
               int ind1 = ln.indexOf("id") + 4;
               for (int i = ind1; i < ln.length(); i++) {
                  if (ln.charAt(i)=='"'){
                     break;
                  }
                  id += ln.charAt(i);
               }
            }
            
            String uuid = UUID.randomUUID().toString();
            
            ArrayList<String> refs = new ArrayList<String>();
            String nxLn = inSS.nextLine();
            while (!nxLn.contains("/way")) {
               if (nxLn.contains("nd")) {
                  String ref = "";
                  int ind2 = nxLn.indexOf("ref") + 5;
                  for (int i = ind2; i < nxLn.length(); i++) {
                     if (nxLn.charAt(i)=='"') {
                        break;
                     }
                     ref += nxLn.charAt(i);
                  }
                  if (uuidMap.containsKey(ref)) {
                     refs.add(ref);
                  }
               }
               nxLn = inSS.nextLine();
            }
            
            Ways[ind] = new Way(uuid, id, refs);
           
            ind++;
         }
      }
      
      PrintWriter outfile = new PrintWriter("Ways.json");
      outfile.println("[");
      int counter2 = 0;
      for (int i = 0; i < ind; i++) {
         if (Ways[i].refs.size() >= 2) {
            if (counter2 >= 1) {
               outfile.println(",");
            }
            outfile.println(" {");
            outfile.println("    \"uuid\": \"" + Ways[i].uuid+"\",");
            //outfile.println("    \"id\": \"" + Ways[i].id+"\",");
            outfile.println("    \"nodeUuidList\": [");
            for (int j = 0; j < Ways[i].refs.size(); j++) {
               String temp = "\",";
               if (j == Ways[i].refs.size() - 1) {
                  temp = "\"";
               }
               outfile.println("       \""+uuidMap.get(Ways[i].refs.get(j)) + temp);
            }
            outfile.println("    ]");
            outfile.print(" }");     
            counter2++;
         }
        
      }
      outfile.println();
      outfile.println("]");
      outfile.flush();
      outfile.close();
   }
   
   public static void main(String[] args) throws IOException {
      //change to txt extension
      String fileName = "map.txt";
      int ind = parseNodes(fileName);
      String nodeFile = "Nodes-ID.txt";
      parseWays(fileName, nodeFile, ind);
   }
}