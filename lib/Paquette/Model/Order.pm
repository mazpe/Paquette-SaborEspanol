package Paquette::Model::Order;

use Moose;
use MyOrder;
use namespace::autoclean;
extends 'Catalyst::Model';

with 'Catalyst::Component::InstancePerContext';

=head1 NAME

Paquette::Model::Order - Catalyst Model

=head1 DESCRIPTION

Catalyst Model.

=head 1 METHODS

=cut

sub build_per_context_instance {
    my ( $self, $c ) = @_;
    
    return MyOrder->new(
        user            => $c->user,
        schema          => $c->model('PaquetteDB')->schema
    );
}

=head1 AUTHOR

Lester Ariel Mesa,,305-402-6717,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
