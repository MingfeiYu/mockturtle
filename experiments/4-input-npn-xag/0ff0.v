module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 ;
  assign n5 = x3 ^ x2 ;
  assign y0 = n5 ;
endmodule
