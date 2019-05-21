# Delete a text between two markers. First marker is deleted the second one is
# not.

/.*>>>2.*/{:loop s/.*\n//g; /.*>>>=.*/!{N; b loop}}