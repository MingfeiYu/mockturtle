module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 , n6 , n7 , n8 , n9 , n10 , n11 ;
  assign n5 = x1 ^ x0 ;
  assign n6 = x3 ^ x2 ;
  assign n8 = n5 & ~n6 ;
  assign n10 = ~x2 & n8 ;
  assign n7 = ~x1 & n6 ;
  assign n9 = ~n5 & n7 ;
  assign n11 = n10 ^ n9 ;
  assign y0 = n11 ;
endmodule
