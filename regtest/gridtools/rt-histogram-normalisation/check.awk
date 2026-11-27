# Independent oracle: steps 1..3 carry weights 2,3,4; 4..6 carry 5,6,7.
function abs(x) { return x<0 ? -x : x }
function fail(message) {
  print "FAILURE: " FILENAME ": " message
  failed=1
}
$1=="#!" && $2=="SET" && $3=="normalisation" {
  seen=1
  weight=$4+0
  next
}
$1 ~ /^#/ || NF==0 { next }
{
  rows++
  if(kind=="header") next
  bin=int(4*$1+0.5)
  expected=0
  if(kind=="strided") {
    if(block==1 && bin==2) expected=1
    if(block==2 && bin==1) expected=5/8
    if(block==2 && bin==2) expected=3/8
    if(block==3 && bin==3) expected=1
  } else if(kind=="limited") {
    if(bin==1) expected=7/14
    if(bin==2) expected=3/14
    if(bin==3) expected=4/14
  } else if(bin>=1 && bin<=3) {
    numerator=block==1 ? bin+1 : bin+4
    expected=kind=="ndata" ? 1/3 : numerator/wantWeight
    if(kind=="raw") expected=numerator
  }
  if(dimension==2 && abs($2-(1-$1))>1e-8) expected=0
  column=dimension==2 ? 3 : 2
  if(kind=="fes") {
    # Empty bins have infinite free energy; compare populated bins only.
    if(expected==0) next
    expected=-log(expected)
  }
  if(kind=="scaled") expected*=2
  if(abs($column-expected)>1e-8) fail("unexpected bin value")
}
END {
  if(rows!=(dimension==2 ? 25 : 5)) fail("wrong grid size")
  if(wantWeight<0) {
    if(seen) fail("invented histogram normalization")
  } else {
    if(!seen) fail("missing normalization")
    else if(abs(weight-wantWeight)>1e-8) fail("wrong block weight")
  }
  exit failed
}
