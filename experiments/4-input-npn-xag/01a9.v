module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 , n6 , n7 , n8 ;
  assign n6 = x0 & x3 ;
  assign n5 = ~x1 & ~x2 ;
  assign n7 = n5 ^ x0 ;
  assign n8 = ~n6 & n7 ;
  assign y0 = n8 ;
endmodule
