module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 , n6 , n7 , n8 ;
  assign n6 = x3 ^ x1 ;
  assign n7 = ~x2 & n6 ;
  assign n5 = ~x0 & ~x3 ;
  assign n8 = n7 ^ n5 ;
  assign y0 = n8 ;
endmodule
