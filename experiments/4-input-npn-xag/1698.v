module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 , n6 , n7 , n8 , n9 , n10 , n11 ;
  assign n5 = x0 & ~x3 ;
  assign n6 = ~x2 & ~n5 ;
  assign n7 = n6 ^ x0 ;
  assign n8 = n6 ^ x3 ;
  assign n9 = x1 & n8 ;
  assign n10 = n7 & ~n9 ;
  assign n11 = n10 ^ x1 ;
  assign y0 = ~n11 ;
endmodule
