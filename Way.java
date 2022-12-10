import java.util.ArrayList;

public class Way {

   public String id;
   public String uuid;
   public ArrayList<String> refs;
   
   public Way(String uuid, String id, ArrayList<String> refs) {
      this.uuid = uuid;
      this.id = id;
      this.refs = refs;
   }



}