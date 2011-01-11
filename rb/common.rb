module Zbroker
   module StatusCode
      UnknownCommand = 500
      Unimplemented  = 501
      RequestTooLong = 502
      NoMoreItem     = 503
      ResponseToLong = 504
      AlreadyOpen    = 505
      OK             = 200
   end
   module Limit
      HeaderSize   = ( 16 )
      PacketIdSize = ( 32 )
      MaxBodySize  = ( 1024 * 1024 )
   end
   module Command
      Open  = 100
      Read  = 101
      Write = 102
   end
   module Purpose
      Read  = 1
      Write = 2
   end
   class ZbrokerError < StandardError
      def self.error_message( v )
         case v
         when StatusCode::Unimplemented
            "Unimplemented"
         when StatusCode::UnknownCommand
            "UnknownCommand"
         when StatusCode::ResponseToLong
            "ResponseToLong"
         when StatusCode::NoMoreItem
            "NoMoreItem"
         when StatusCode::AlreadyOpen
            "AlreadyOpen"
         else
            raise ZbrokerError,"Invalid error code:#{v}"
         end
      end
   end 
end
