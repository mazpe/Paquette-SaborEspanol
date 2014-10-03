package Paquette::Controller::Root;

use strict;
use warnings;
use parent 'Catalyst::Controller';
use Data::Dumper;

#
# Sets the actions in this controller to be registered with no prefix
# so they function identically to actions created in MyApp.pm
#
__PACKAGE__->config->{namespace} = '';

=head1 NAME

Paquette::Controller::Root - Root Controller for Paquette

=head1 DESCRIPTION

[enter your description here]

=head1 METHODS

=cut

=head2 index

=cut

sub index :Path :Args(0) {
    my ( $self, $c ) = @_;
    my $categories;
    my $cart_size;

    $categories = [$c->model('PaquetteDB::Categories')->search(
        { parent_id => 0, active => 1 },
    )];
    $c->stash->{'categories'}    = $categories;

    $cart_size = $c->model('Cart')->count_items_in_cart;
    $c->stash->{cart_size}  = $cart_size;

    #$c->response->redirect($c->uri_for($c->controller('Customers')->action_for(
    #    'pre_registration'
    #)));
    
    $c->stash->{template} = "index.tt2";
}

sub default :Path {
    my ( $self, $c ) = @_;

    $c->response->body( 'Page not found' );
    $c->response->status(404);
}

=head2 end

Attempt to render a view, if needed.

=cut

sub end : ActionClass('RenderView') {}

=head1 AUTHOR

root

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
