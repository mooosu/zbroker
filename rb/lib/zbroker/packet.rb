module Zbroker
   class Packet
      attr_reader :body
      attr_reader :packet_id
      def initialize(body="", header_size=Limit::HeaderSize,
                     packet_id_size=Limit::PacketIdSize)

         @header =nil
         @body = body

         @header_size = header_size
         @packet_id_size =  packet_id_size
         @header_format = "%#{@header_size}d"
         @packet_id = 'x'*packet_id_size
      end
      def data
         encode_header unless @header
         @header+@packet_id+@body
      end
      def body=(value)
         M2::Dia.assert( value.size <= Limit::MaxBodySize )
         @body = value
      end
      def packet_id=(value)
         tmp = value.to_s
         M2::Dia.assert(tmp.size <=@packet_id_size,
                        "request header to long(>#{@packet_id_size})")
         @packet_id = tmp
      end
      def size
         @header_size + @packet_id_size + @body.size
      end
      def decode_header
         @packet_id = @body[0,@packet_id_size]
         @body= @body[@packet_id_size..-1]
      end
      def encode_header
         @header = sprintf(@header_format, @body.size+@packet_id_size)
      end
   end
end
