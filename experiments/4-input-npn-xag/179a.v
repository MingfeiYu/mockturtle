module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 , n6 , n7 , n8 , n9 , n10 , n11 ;
  assign n7 = x1 & x3 ;
  assign n6 = ~x1 & x2 ;
  assign n9 = n6 ^ x0 ;
  assign n10 = ~n7 & n9 ;
  assign n5 = ~x0 & ~x2 ;
  assign n8 = x3 & n5 ;
  assign n11 = n10 ^ n8 ;
  assign y0 = n11 ;
endmodule
