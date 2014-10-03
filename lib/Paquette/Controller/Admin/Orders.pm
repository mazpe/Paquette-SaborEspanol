package Paquette::Controller::Admin::Orders;

use Moose;
BEGIN { extends 'Catalyst::Controller' }
use Data::Dumper;

=head1 NAME

Paquette::Controller::Admin::Orders - Catalyst Controller

=head1 DESCRIPTION

Catalyst Controller.

=head1 METHODS

=cut


=head2 index

=cut

sub auto : Private {
    my ( $self, $c ) =  @_;

    $c->stash->{wrapper_admin} = 1;
}

sub index :Chained('based') :Path :Args(0) {
    my ( $self, $c ) = @_;

    my $orders = [$c->model('PaquetteDB::Orders')->all];

    $c->stash->{orders} = $orders;

    $c->stash->{template} = 'admin/orders_list.tt2';

    return;
}

sub base :Chained('/') :PathPart('admin/orders') :CaptureArgs(0) {
    my ( $self, $c ) = @_;

}

sub load : Chained('base') :PathPart('') :CaptureArgs(1) {
    my ( $self, $c, $id ) = @_;
    my $order;

    if($id) {

        $c->stash->{order_id} = $id;
        $order    = $c->model('PaquetteDB::Orders')->find($id);

    } else {

        $c->response->status(404);
        $c->detach;

    }

    if ($order) {

        $c->stash->{'order'} = $order;

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}

sub create : Local {
    my ( $self, $c ) = @_;
    my $row;
    my $form;

    # Get a new empty row
    $row = $c->model('PaquetteDB::Orders')->new_result({});

    # Set our template and form to use
    $c->stash(
        template    => 'admin/order.tt2',
        form        => $self->order_form,
    );

    # Process our form
    $form =  $self->order_form->process (
        item            => $row,
        params          => $c->req->params,
    );

    # If the form has been submited sucessfully, then redirect to confirm page
    if ($form) {
        $c->res->redirect( $c->uri_for($self->action_for('index')) );
    }

}

sub edit : Chained('load') : PathPart('edit') : Args(0) {
    my ( $self, $c ) = @_;
    my $order_id;
    my $order;

    $order_id = $c->stash->{order_id};
    # Get a new empty row
    $order = $c->model('Order')->get_order({ order_id => $order_id });

    # Set our template and form to use
    $c->stash(
        order       => $order,
        cc_last4    => substr($order->credit_card_number, -4),
        sum_weight  
            => $c->model('PaquetteDB::OrdersItem')->sum_weight($order_id),
        order_items => $c->model('Order')->get_items_in_order($order_id),
        template    => 'admin/order.tt2',
    );

    # If the form has been submited sucessfully, then redirect to confirm page
    if ($c->req->params->{submit}) {
        $c->res->redirect( $c->uri_for($self->action_for('index')) );
    }
}

sub delete : Chained('load') : PathPart('delete') : Args(0) {
    my ( $self, $c ) = @_;
    my $cmd;
    my $order = $c->stash->{order};

    if ($order) {

        my $is_deleted = $c->model('Order')->destroy_order($order->id);
        
        $c->response->redirect(
            $c->uri_for( $self->action_for('index')) . '/' );

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}

sub change_status : Chained('load') : PathPart('change_status') : Args(0) {
    my ( $self, $c ) = @_;
    my $cmd;

    my $order = $c->stash->{order};

    $order->update( { 'status' => $c->req->params->{status} } );

    $c->response->redirect(
        $c->uri_for( $self->action_for('index')) . '/' );

}


=head1 AUTHOR

Lester Ariel Mesa,,305-402-6717,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
