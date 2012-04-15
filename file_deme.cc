#include "ace/FILE_IO.h"
#include "ace/SOCK_Stream.h"

class ACE_Message_Block; // Forward declaration.

class Logging_Handler
{
protected:
  // Reference to a log file.
  ACE_FILE_IO &log_file_; 

  // Connected to the client.
  ACE_SOCK_Stream logging_peer_; 
	// Receive one log record from a connected client.  <mblk> 
  // contains the hostname, <mblk->cont()> contains the log 
  // record header (the byte order & the length) & the data. 
  int recv_log_record (ACE_Message_Block *&mblk);

  // Write one record to the log file. The <mblk> contains the 
  // hostname & the <mblk->cont> contains the log record. 
  int write_log_record (ACE_Message_Block *mblk);

  // Log one record by calling <recv_log_record> and
  // <write_log_record>. 
  int log_record (); 
};



int Logging_Handler::recv_log_record (ACE_Message_Block *&mblk)
 {
   ACE_INET_Addr peer_addr;
   logging_peer_.get_remote_addr (peer_addr);
   mblk = new ACE_Message_Block (MAXHOSTNAMELEN + 1);
   peer_addr.get_host_name (mblk->wr_ptr (), MAXHOSTNAMELEN);
   mblk->wr_ptr (strlen (mblk->wr_ptr ()) + 1); // Go past name.

   ACE_Message_Block *payload =
     new ACE_Message_Block (ACE_DEFAULT_CDR_BUFSIZE);
   // Align Message Block for a CDR stream.
   ACE_CDR::mb_align (payload);

  if (logging_peer_.recv_n (payload->wr_ptr (), 8) == 8) {
     payload->wr_ptr (8);     // Reflect addition of 8 bytes.

     ACE_InputCDR cdr (payload);
     ACE_CDR::Boolean byte_order;
     // Use helper method to disambiguate booleans from chars.
     cdr >> ACE_InputCDR::to_boolean (byte_order);
     cdr.reset_byte_order (byte_order);
 
     ACE_CDR::ULong length;
     cdr >> length;

     payload->size (8 + ACE_CDR::MAX_ALIGNMENT + length);

     if (logging_peer_.recv_n (payload->wr_ptr(), length) > 0) {
      payload->wr_ptr (length);   // Reflect additional bytes.
       mblk->cont (payload); // Chain the header & payload.
       return length; // Return length of the log record.
     }
   }
   payload->release ();
   mblk->release ();
   payload = mblk = 0;
   return -1;
 }


int Logging_Handler::write_log_record (ACE_Message_Block *mblk)
  {
    if (log_file_->send_n (mblk) == -1) return -1;
 
    if (ACE::debug ()) {
      ACE_InputCDR cdr (mblk->cont ()); 
      ACE_CDR::Boolean byte_order;
      ACE_CDR::ULong length;
      cdr >> ACE_InputCDR::to_boolean (byte_order);
     cdr.reset_byte_order (byte_order);
     cdr >> length;
     ACE_Log_Record log_record;
   cdr >> log_record;  // Extract the <ACE_log_record>.
     log_record.print (mblk->rd_ptr (), 1, cerr);
  }
 
  return mblk->total_length ();
 }




int Logging_Handler::log_record ()
{
  ACE_Message_Block *mblk = 0;
  if (recv_log_record (mblk) == -1)
    return -1;
  else {
    int result = write_log_record (mblk);
    mblk->release (); // Free up the entire contents.
    return result == -1 ? -1 : 0;
  }
}
