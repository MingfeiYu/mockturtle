module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 , n6 , n7 , n8 , n9 , n10 ;
  assign n5 = x0 & x2 ;
  assign n6 = ~x1 & ~n5 ;
  assign n7 = n6 ^ x0 ;
  assign n8 = n7 ^ x2 ;
  assign n9 = n7 ^ x3 ;
  assign n10 = ~n8 & n9 ;
  assign y0 = n10 ;
endmodule
