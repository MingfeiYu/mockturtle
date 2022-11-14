module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 , n6 , n7 , n8 , n9 , n10 , n11 ;
  assign n5 = x2 ^ x1 ;
  assign n6 = n5 ^ x0 ;
  assign n7 = n6 ^ x3 ;
  assign n9 = n5 & ~n7 ;
  assign n8 = n7 ^ x1 ;
  assign n10 = n6 & ~n8 ;
  assign n11 = ~n9 & ~n10 ;
  assign y0 = ~n11 ;
endmodule
