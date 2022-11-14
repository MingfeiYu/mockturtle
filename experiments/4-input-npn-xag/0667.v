module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 , n6 , n7 , n8 , n9 , n10 ;
  assign n5 = x3 ^ x2 ;
  assign n7 = x2 & ~n5 ;
  assign n6 = ~x0 & ~n5 ;
  assign n8 = ~x1 & ~n6 ;
  assign n9 = n8 ^ x0 ;
  assign n10 = ~n7 & ~n9 ;
  assign y0 = n10 ;
endmodule
