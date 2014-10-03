package Paquette::Schema::ResultSet::Promotion;

use strict;
use warnings;
use base 'DBIx::Class::ResultSet';

=head1 NAME

Paquette::Schema::ResultSet::Promotion - ResultSet 

=head1 DESCRIPTION

Promotions ResultSet

=cut

sub get_promotion {
    my ( $self, $args ) = @_;
    my $row;
    $row = $self->find({ code => $args });

    return $row;
}
    


=head1 AUTHOR

Lester Ariel Mesa,,305-402-6717,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;

