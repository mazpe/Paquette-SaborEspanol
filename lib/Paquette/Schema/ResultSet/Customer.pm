package Paquette::Schema::ResultSet::Customer;

use strict;
use warnings;
use base 'DBIx::Class::ResultSet';

=head1 NAME

Paquette::Schema::ResultSet::Customer - ResultSet 

=head1 DESCRIPTION

Customers ResultSet

=cut

sub create_customer {
    my ( $self, %args ) = @_;

    my $customer = $self->find_or_create( 
        {
            bill_first_name     => $args{'bill_first_name'},
            bill_last_name      => $args{'bill_last_name'},
            email               => $args{'email'},
        },
        { key => 'email' },

    );

    return $customer ? $customer : 0;
}

=head1 AUTHOR

Lester Ariel Mesa,,305-402-6717,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;

